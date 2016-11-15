/*
 *  Drm class
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 *
 */

#include "Drm.hpp"

#include <chrono>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>

#include <xen/be/Log.hpp>

using std::advance;
using std::chrono::milliseconds;
using std::exception;
using std::mutex;
using std::shared_ptr;
using std::lock_guard;
using std::string;
using std::this_thread::sleep_for;
using std::thread;
using std::to_string;
using std::unique_ptr;

namespace Drm {

const uint32_t cInvalidId = 0;

/***************************************************************************//**
 * ModeResource
 ******************************************************************************/

ModeResource::ModeResource(int fd)
{
	DLOG("ModeResource", DEBUG) << "Create ModeResource";

	mRes = drmModeGetResources(fd);

	if (!mRes)
	{
		throw DrmException("Cannot retrieve DRM resources");
	}
}

ModeResource::~ModeResource()
{
	DLOG("ModeResource", DEBUG) << "Delete ModeResource";

	if (mRes)
	{
		drmModeFreeResources(mRes);
	}
}

const drmModeResPtr ModeResource::operator->() const
{
	return mRes;
}

const drmModeRes& ModeResource::operator*() const
{
	return *mRes;
}

/***************************************************************************//**
 * ModeConnector
 ******************************************************************************/

ModeConnector::ModeConnector(int fd, int connectorId)
{
	DLOG("ModeConnector", DEBUG) << "Create ModeConnector " << connectorId;

	mConnector = drmModeGetConnector(fd, connectorId);

	if (!mConnector)
	{
		throw DrmException("Cannot retrieve DRM connector");
	}
}

ModeConnector::~ModeConnector()
{
	if (mConnector)
	{
		DLOG("ModeConnector", DEBUG) << "Delete ModeConnector "
									<< mConnector->connector_id;

		drmModeFreeConnector(mConnector);
	}
}

const drmModeConnectorPtr ModeConnector::operator->() const
{
	return mConnector;
}

const drmModeConnector& ModeConnector::operator*() const
{
	return *mConnector;
}

/***************************************************************************//**
 * ModeEncoder
 ******************************************************************************/

ModeEncoder::ModeEncoder(int fd, int encoderId)
{
	mEncoder = drmModeGetEncoder(fd, encoderId);

	if (!mEncoder)
	{
		throw DrmException("Cannot retrieve DRM encoder: " + to_string(encoderId));
	}

	DLOG("ModeEncoder", DEBUG) << "Create ModeEncoder, id: " << encoderId
							  << ", crtc id: " << mEncoder->crtc_id;
}

ModeEncoder::~ModeEncoder()
{
	if (mEncoder)
	{
		DLOG("ModeEncoder", DEBUG) << "Delete ModeEncoder "
								  << mEncoder->encoder_id;

		drmModeFreeEncoder(mEncoder);
	}
}

const drmModeEncoderPtr ModeEncoder::operator->() const
{
	return mEncoder;
}

const drmModeEncoder& ModeEncoder::operator*() const
{
	return *mEncoder;
}

/***************************************************************************//**
 * Dumb
 ******************************************************************************/

Dumb::Dumb(DrmDevice& drm, uint32_t width, uint32_t height, uint32_t bpp) :
	mDrm(drm),
	mHandle(cInvalidId),
	mPitch(0),
	mWidth(width),
	mHeight(height),
	mSize(0),
	mBuffer(nullptr)
{
	try
	{
		drm_mode_create_dumb creq {0};

		creq.width = width;
		creq.height = height;
		creq.bpp = bpp;

		auto ret = drmIoctl(mDrm.getFd(), DRM_IOCTL_MODE_CREATE_DUMB, &creq);

		if (ret < 0)
		{
			throw DrmException("Cannot create dumb buffer");
		}

		mPitch = creq.pitch;
		mSize = creq.size;
		mHandle = creq.handle;

		drm_mode_map_dumb mreq {0};

		mreq.handle = mHandle;

		ret = drmIoctl(mDrm.getFd(), DRM_IOCTL_MODE_MAP_DUMB, &mreq);

		if (ret < 0)
		{
			throw DrmException("Cannot map dumb buffer.");
		}

		auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
						mDrm.getFd(), mreq.offset);

		if (map == MAP_FAILED)
		{
			throw DrmException("Cannot mmap dumb buffer");
		}

		mBuffer = map;

		DLOG("Dumb", DEBUG) << "Create dumb, handle: " << mHandle << ", size: "
						   << mSize << ", pitch: " << mPitch;

	}
	catch(const exception& e)
	{
		release();

		throw;
	}
}

Dumb::~Dumb()
{
	release();

	DLOG("Dumb", DEBUG) << "Delete dumb, handle: " << mHandle;
}

void Dumb::release()
{
	if (mBuffer)
	{
		munmap(mBuffer, mSize);
	}

	if (mHandle != cInvalidId)
	{
		drm_mode_destroy_dumb dreq {0};

		dreq.handle = mHandle;

		if (drmIoctl(mDrm.getFd(), DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) < 0)
		{
			DLOG("Dumb" , ERROR) << "Cannot destroy dumb";
		}
	}
}

/***************************************************************************//**
 * FrameBuffer
 ******************************************************************************/

FrameBuffer::FrameBuffer(DrmDevice& drm, Dumb& dumb, uint32_t width,
						 uint32_t height, uint32_t pixelFormat,
						 uint32_t pitch) :
	mDrm(drm),
	mDumb(dumb),
	mId(cInvalidId),
	mFlipPending(false)
{
	uint32_t handles[4], pitches[4], offsets[4] = {0};

	handles[0] = mDumb.getHandle();
	pitches[0] = pitch;

	auto ret = drmModeAddFB2(mDrm.getFd(), width, height, pixelFormat, handles,
							 pitches, offsets, &mId, 0);

	DLOG("FrameBuffer", DEBUG) << "Create frame buffer, handle: " << handles[0]
							  << ", id: " << mId;

	if (ret)
	{
		throw DrmException ("Cannot create frame buffer: " +
							string(strerror(errno)));
	}

}

FrameBuffer::~FrameBuffer()
{
	if (mFlipPending)
	{
		sleep_for(milliseconds(100));
	}

	if (mFlipPending)
	{
		LOG("FrameBuffer", ERROR) << "Delete frame buffer on pending flip";
	}

	if (mId != cInvalidId)
	{
		DLOG("FrameBuffer", DEBUG) << "Delete frame buffer, handle: "
								  << getHandle() << ", id: " << mId;

		drmModeRmFB(mDrm.getFd(), mId);
	}
}

bool FrameBuffer::pageFlip(uint32_t crtc, FlipCallback cbk)
{
	if (mDrm.isStopped())
	{
		DLOG("FrameBuffer", WARNING) << "Page flip when DRM is stopped";

		return false;
	}

	auto ret = drmModePageFlip(mDrm.getFd(), crtc, mId,
							   DRM_MODE_PAGE_FLIP_EVENT, this);

	if (ret)
	{
		throw DrmException("Cannot flip CRTC: " + mId);
	}

	mDrm.pageFlipScheduled();
	mFlipPending = true;
	mFlipCallback = cbk;

	return true;
}

void FrameBuffer::flipFinished()
{
	mDrm.pageFlipDone();

	if (!mFlipPending)
	{
		LOG("FrameBuffer", ERROR) << "Not expected flip event";

		return;
	}

	mFlipPending = false;

	if (mFlipCallback)
	{
		mFlipCallback();
	}
}
/***************************************************************************//**
 * Connector
 ******************************************************************************/

Connector::Connector(DrmDevice& dev, int connectorId) :
	mDev(dev),
	mFd(dev.getFd()),
	mCrtcId(cInvalidId),
	mConnector(mFd, connectorId),
	mSavedCrtc(nullptr),
	mLog("Connector(" + to_string(connectorId) + ")")
{
	LOG(mLog, DEBUG) << "Create connector";
}

Connector::~Connector()
{
	release();

	LOG(mLog, DEBUG) << "Delete connector";
}

void Connector::init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
					 uint32_t bpp, uint32_t fbId)
{

	if (mConnector->connection != DRM_MODE_CONNECTED)
	{
		throw DrmException("Connector is not connected");
	}

	if (mCrtcId != cInvalidId)
	{
		throw DrmException("Already initialized");
	}

	LOG(mLog, DEBUG) << "Init, w: " << width << ", h: " << height
					 << ", bpp " << bpp << ", fb id: " << fbId;

	mCrtcId = findCrtcId();

	if (mCrtcId == cInvalidId)
	{
		throw DrmException("Cannot find CRTC for connector");
	}

	auto mode = findMode(width, height);

	if (!mode)
	{
		throw DrmException("Unsupported mode");
	}

	mSavedCrtc = drmModeGetCrtc(mFd, mCrtcId);

	if (drmModeSetCrtc(mFd, mCrtcId, fbId, 0, 0,
					   &mConnector->connector_id, 1, mode))
	{
		throw DrmException("Cannot set CRTC for connector");
	}
}

void Connector::release()
{
	mCrtcId = cInvalidId;

	if (mSavedCrtc)
	{
		LOG(mLog, DEBUG) << "Release";

		drmModeSetCrtc(mFd, mSavedCrtc->crtc_id, mSavedCrtc->buffer_id,
					   mSavedCrtc->x, mSavedCrtc->y, &mConnector->connector_id,
					   1, &mSavedCrtc->mode);

		drmModeFreeCrtc(mSavedCrtc);

		mSavedCrtc = nullptr;
	}
}

uint32_t Connector::findCrtcId()
{
	auto crtcId = getAssignedCrtcId();

	if (crtcId != cInvalidId)
	{
		return crtcId;
	}

	return findMatchingCrtcId();
}

bool Connector::isCrtcIdUsedByOther(uint32_t crtcId)
{
	for (size_t i = 0; i < mDev.getConnectorsCount(); i++)
	{
		if (crtcId == mDev.getConnectorByIndex(i).getCrtcId())
		{
			LOG(mLog, DEBUG) << "Crtc id is used by other connector";

			return true;
		}
	}

	return false;
}

uint32_t Connector::getAssignedCrtcId()
{
	auto encoderId = mConnector->encoder_id;

	if (encoderId != cInvalidId)
	{
		ModeEncoder encoder(mFd, encoderId);

		auto crtcId = encoder->crtc_id;

		if (crtcId != cInvalidId)
		{

			if (isCrtcIdUsedByOther(crtcId))
			{
				return cInvalidId;
			}

			LOG(mLog, DEBUG) << "Assigned crtc id: " << crtcId;

			return crtcId;
		}
	}

	return cInvalidId;
}

uint32_t Connector::findMatchingCrtcId()
{
	for (int encIndex = 0; encIndex < mConnector->count_encoders; encIndex++)
	{
		ModeEncoder encoder(mFd, mConnector->encoders[encIndex]);

		for (int crtcIndex = 0; crtcIndex < mDev.getCtrcsCount(); crtcIndex++)
		{
			if (!(encoder->possible_crtcs & (1 << crtcIndex)))
			{
				continue;
			}

			auto crtcId = mDev.getCtrcIdByIndex(crtcIndex);

			if (isCrtcIdUsedByOther(crtcId))
			{
				continue;
			}

			LOG(mLog, DEBUG) << "Matched crtc found: " << crtcId;

			return crtcId;
		}
	}

	return cInvalidId;
}

drmModeModeInfoPtr Connector::findMode(uint32_t width, uint32_t height)
{
	for (int i = 0; i < mConnector->count_modes; i++)
	{
		if (mConnector->modes[i].hdisplay == width &&
			mConnector->modes[i].vdisplay == height)
		{
			LOG(mLog, DEBUG) << "Found mode: " << mConnector->modes[i].name;

			return &mConnector->modes[i];
		}
	}

	return nullptr;
}

/***************************************************************************//**
 * Drm
 ******************************************************************************/

DrmDevice::DrmDevice(const string& name) try :
	mName(name),
	mFd(-1),
	mTerminate(true),
	mNumFlipPages(0),
	mLog("DRM(" + mName + ")")
{
	LOG(mLog, DEBUG) << "Create Drm card";

	init();
}
catch(const exception& e)
{
	LOG(mLog, ERROR) << e.what();

	release();

	throw;
}

DrmDevice::~DrmDevice()
{
	stop();

	release();

	LOG(mLog, DEBUG) << "Delete Drm card";
}

/***************************************************************************//**
 * Public
 ******************************************************************************/
Dumb& DrmDevice::createDumb(uint32_t width, uint32_t height, uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	Dumb* dumb = new Dumb(*this, width, height, bpp);

	mDumbs.emplace(dumb->getHandle(), unique_ptr<Dumb>(dumb));

	LOG(mLog, DEBUG) << "Create dumb: " << dumb->getHandle();

	return *dumb;
}

void DrmDevice::deleteDumb(uint32_t handle)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Delete dumb " << handle;

	auto dumbIter = mDumbs.find(handle);

	if (dumbIter == mDumbs.end())
	{
		throw DrmException("Dumb handler not found");
	}

	auto fbIter = mFrameBuffers.begin();

	while (fbIter != mFrameBuffers.end())
	{
		if (dumbIter->second->getHandle() == fbIter->second->getHandle())
		{
			fbIter = mFrameBuffers.erase(fbIter);
		}
		else
		{
			++fbIter;
		}
	}
}

FrameBuffer& DrmDevice::createFrameBuffer(Dumb& dumb, uint32_t width,
									uint32_t height, uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	FrameBuffer* frameBuffer = new FrameBuffer(*this, dumb,
											   width, height, pixelFormat,
											   dumb.getPitch());

	mFrameBuffers.emplace(frameBuffer->getId(),
						  unique_ptr<FrameBuffer>(frameBuffer));

	LOG(mLog, DEBUG) << "Create frame buffer " << frameBuffer->getId();

	return *frameBuffer;
}

void DrmDevice::deleteFrameBuffer(uint32_t id)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Delete frame buffer " << id;

	auto fbIter = mFrameBuffers.find(id);

	if (fbIter == mFrameBuffers.end())
	{
		throw DrmException("Frame buffer id not found");
	}

	mFrameBuffers.erase(fbIter);
}

void DrmDevice::start()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Start";

	if (!mTerminate)
	{
		return;
	}

	mTerminate = false;

	mThread = thread(&DrmDevice::eventThread, this);
}

void DrmDevice::stop()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Stop";

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

Connector& DrmDevice::getConnectorById(uint32_t id)
{
	auto iter = mConnectors.find(id);

	if (iter == mConnectors.end())
	{
		throw DrmException("Wrong connector id " + to_string(id));
	}

	return *iter->second;
}

Connector& DrmDevice::getConnectorByIndex(uint32_t index)
{
	if (index >= mConnectors.size())
	{
		throw DrmException("Wrong connector index " + to_string(index));
	}

	auto iter = mConnectors.begin();

	advance(iter, index);

	return *iter->second;
}

size_t DrmDevice::getConnectorsCount()
{
	return mConnectors.size();
}

/***************************************************************************//**
 * Private
 ******************************************************************************/

void DrmDevice::init()
{
	mFd = open(mName.c_str(), O_RDWR | O_CLOEXEC);

	if (mFd < 0)
	{
		throw DrmException("Cannot open " + mName);
	}

	uint64_t hasDumb = false;

	if (drmGetCap(mFd, DRM_CAP_DUMB_BUFFER, &hasDumb) < 0 || !hasDumb)
	{
		throw DrmException("Drm device does not support dumb buffers");
	}

	mRes.reset(new ModeResource(mFd));

	for (int i = 0; i < (*mRes)->count_connectors; i++)
	{
		Connector* connector = new Connector(*this, (*mRes)->connectors[i]);

		mConnectors.emplace((*mRes)->connectors[i],
							unique_ptr<Connector>(connector));
	}
}

void DrmDevice::release()
{
	mFrameBuffers.clear();

	mDumbs.clear();

	mRes.release();

	mConnectors.clear();

	if (mFd >= 0)
	{
		close(mFd);
	}
}

void DrmDevice::eventThread()
{
	pollfd fds;

	fds.fd = mFd;
	fds.events = POLLIN;

	drmEventContext ev { 0 };

	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = handleFlipEvent;

	while(!mTerminate)
	{
		auto ret = poll(&fds, 1, cPoolEventTimeoutMs);

		if (ret < 0)
		{
			LOG(mLog, ERROR) << "Can't poll events";
		}

		if (ret > 0)
		{
			drmHandleEvent(mFd, &ev);
		}
	}

	while(mNumFlipPages)
	{
		drmHandleEvent(mFd, &ev);
	}
}

void DrmDevice::handleFlipEvent(int fd, unsigned int sequence,
								unsigned int tv_sec, unsigned int tv_usec,
								void *user_data)
{
	if (user_data)
	{
		static_cast<FrameBuffer*>(user_data)->flipFinished();
	}
}

}
