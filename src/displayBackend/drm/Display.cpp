/*
 *  Device class
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

#include "Display.hpp"

#include <fcntl.h>

#include <xf86drm.h>

#include <drm/xen_zcopy_drm.h>
#include <xen/be/Log.hpp>

#include "Dumb.hpp"
#include "DumbZCopyBack.hpp"
#include "DumbZCopyFront.hpp"

using std::dynamic_pointer_cast;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;

using XenBackend::PollFd;

using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;
using DisplayItf::GrantRefs;

namespace Drm {

const uint32_t cInvalidId = 0;

/*******************************************************************************
 * Device
 ******************************************************************************/

Display::Display(const string& name) :
	mName(name),
	mFd(-1),
	mZeroCopyFd(-1),
	mStarted(false),
	mLog("Drm")
{
	try
	{
		init();
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

Display::~Display()
{
	stop();

	release();

	LOG(mLog, DEBUG) << "Delete Drm card: " << mName;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

drm_magic_t Display::getMagic()
{
	lock_guard<mutex> lock(mMutex);

	drm_magic_t magic = 0;

	if (drmGetMagic(mFd, &magic) < 0)
	{
		throw Exception("Can't get magic");
	}

	LOG(mLog, DEBUG) << "Get magic: " << magic;

	return magic;
}

Drm::ConnectorPtr Display::getConnectorByIndex(uint32_t index)
{
	lock_guard<mutex> lock(mMutex);

	if (index >= mConnectors.size())
	{
		throw Exception("Wrong connector index " + to_string(index));
	}

	auto iter = mConnectors.begin();

	advance(iter, index);

	return iter->second;
}

size_t Display::getConnectorsCount()
{
	lock_guard<mutex> lock(mMutex);

	return mConnectors.size();
}

void Display::autoCreateConnectors()
{
	lock_guard<mutex> lock(mMutex);

	int id = 0;

	for (int i = 0; i < (*mRes)->count_connectors; i++)
	{
		ModeConnector connector(mFd, (*mRes)->connectors[i]);

		if (connector->connection == DRM_MODE_CONNECTED)
		{
			Drm::ConnectorPtr connector(
					new Connector(*this, (*mRes)->connectors[i]));

			mConnectors.emplace(id++, connector);
		}
	}

	LOG(mLog, DEBUG) << "Created connectors: " << id;
}

DisplayItf::ConnectorPtr Display::createConnector(uint32_t id, uint32_t drmId)
{
	lock_guard<mutex> lock(mMutex);

	for (int i = 0; i < (*mRes)->count_connectors; i++)
	{
		if (drmId == (*mRes)->connectors[i])
		{
			Drm::ConnectorPtr connector(
					new Connector(*this, (*mRes)->connectors[i]));

			mConnectors.emplace(id, connector);

			return connector;
		}
	}

	throw Exception("No DRM connector id found: " + to_string(drmId));
}

void Display::start()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Start";

	if (mStarted)
	{
		LOG(mLog, WARNING) << "Already started";

		return;
	}

	mStarted = false;

	mThread = thread(&Display::eventThread, this);
}

void Display::stop()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Stop";

	mPollFd->stop();

	if (mThread.joinable())
	{
		mThread.join();
	}
}

DisplayItf::ConnectorPtr Display::getConnectorById(uint32_t id)
{
	lock_guard<mutex> lock(mMutex);

	auto iter = mConnectors.find(id);

	if (iter == mConnectors.end())
	{
		throw Exception("Wrong connector id " + to_string(id));
	}

	return dynamic_pointer_cast<Connector>(iter->second);
}

DisplayBufferPtr Display::createDisplayBuffer(uint32_t width, uint32_t height,
											 uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create display buffer";

	return DisplayBufferPtr(new Dumb(mFd, width, height, bpp));
}

DisplayBufferPtr Display::createDisplayBuffer(
		uint32_t width, uint32_t height, uint32_t bpp,
		domid_t domId, GrantRefs& refs, bool allocRefs)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create display buffer w/ zero copy " << isZeroCopySupported();

	if (isZeroCopySupported())
	{
		DLOG(mLog, DEBUG) << "mFd: " << mFd;
		DLOG(mLog, DEBUG) << "mZeroCopyFd: " << mZeroCopyFd;

		if (allocRefs)
		{
			return DisplayBufferPtr(new DumbZCopyBack(mFd, mZeroCopyFd,
													  width, height, bpp,
													  domId, refs));
		}
		else
		{
			return DisplayBufferPtr(new DumbZCopyFront(mFd, mZeroCopyFd,
													   width, height, bpp,
													   domId, refs));
		}
	}
	else
	{
		return DisplayBufferPtr(new Dumb(mFd, width, height, bpp, domId, refs));
	}
}

FrameBufferPtr Display::createFrameBuffer(DisplayBufferPtr displayBuffer,
										 uint32_t width, uint32_t height,
										 uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create frame buffer";

	return FrameBufferPtr(new FrameBuffer(mFd, displayBuffer, width,
										  height, pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Display::init()
{
	mFd = open(mName.c_str(), O_RDWR | O_CLOEXEC);

	if (mFd < 0)
	{
		throw Exception("Cannot open DRM device: " + mName);
	}

	mPollFd.reset(new PollFd(mFd, POLLIN));

	uint64_t hasDumb = false;

	if (drmGetCap(mFd, DRM_CAP_DUMB_BUFFER, &hasDumb) < 0 || !hasDumb)
	{
		throw Exception("Drm device does not support dumb buffers");
	}

	mRes.reset(new ModeResource(mFd));

	for (int i = 0; i < (*mRes)->count_connectors; i++)
	{
		ModeConnector connector(mFd, (*mRes)->connectors[i]);

		LOG(mLog, DEBUG) << "Available connector: " << connector->connector_id
						 << ", connected: "
						 << (connector->connection == DRM_MODE_CONNECTED);
	}

	mZeroCopyFd = drmOpen(XENDRM_ZCOPY_DRIVER_NAME, NULL);

	if (mZeroCopyFd < 0)
	{
		LOG(mLog, WARNING) << "Can't open zero copy driver. "
						   << "Zero copy functionality will be disabled.";
	}

	LOG(mLog, DEBUG) << "Create Drm card: " << mName;
}

void Display::release()
{
	mRes.reset();

	mConnectors.clear();

	if (mZeroCopyFd >= 0)
	{
		drmClose(mZeroCopyFd);
	}

	if (mFd >= 0)
	{
		drmClose(mFd);
	}
}

void Display::eventThread()
{
	try
	{
		drmEventContext ev { 0 };

		ev.version = DRM_EVENT_CONTEXT_VERSION;
		ev.page_flip_handler = handleFlipEvent;

		while(mPollFd->poll())
		{
			drmHandleEvent(mFd, &ev);
		}
	}
	catch(const std::exception& e)
	{
		LOG(mLog, ERROR) << e.what();
	}

	mStarted = false;
}

void Display::handleFlipEvent(int fd, unsigned int sequence,
								unsigned int tv_sec, unsigned int tv_usec,
								void *user_data)
{
	if (user_data)
	{
		static_cast<Connector*>(user_data)->flipFinished();
	}
}

}
