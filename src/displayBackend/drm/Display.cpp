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
#ifdef WITH_ZCOPY
#include <drm/xen_zcopy_drm.h>
#endif

#include <xen/be/Log.hpp>

#include "Dumb.hpp"
#include "DumbZCopyBack.hpp"
#include "DumbZCopyFront.hpp"

using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;
using std::unordered_map;

using XenBackend::PollFd;

using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;
using DisplayItf::GrantRefs;

namespace Drm {

const uint32_t cInvalidId = 0;

unordered_map<int, string> Display::sConnectorNames =
{
	{ DRM_MODE_CONNECTOR_Unknown,		"unknown" },
	{ DRM_MODE_CONNECTOR_VGA,			"VGA" },
	{ DRM_MODE_CONNECTOR_DVII,			"DVI-I" },
	{ DRM_MODE_CONNECTOR_DVID,			"DVI-D" },
	{ DRM_MODE_CONNECTOR_DVIA,			"DVI-A" },
	{ DRM_MODE_CONNECTOR_Composite,		"composite" },
	{ DRM_MODE_CONNECTOR_SVIDEO,		"s-video" },
	{ DRM_MODE_CONNECTOR_LVDS,			"LVDS" },
	{ DRM_MODE_CONNECTOR_Component,		"component" },
	{ DRM_MODE_CONNECTOR_9PinDIN,		"9-pin DIN" },
	{ DRM_MODE_CONNECTOR_DisplayPort,	"DP" },
	{ DRM_MODE_CONNECTOR_HDMIA,			"HDMI-A" },
	{ DRM_MODE_CONNECTOR_HDMIB,			"HDMI-B" },
	{ DRM_MODE_CONNECTOR_TV,			"TV" },
	{ DRM_MODE_CONNECTOR_eDP,			"eDP" },
	{ DRM_MODE_CONNECTOR_VIRTUAL,		"Virtual" },
	{ DRM_MODE_CONNECTOR_DSI,			"DSI" },
};

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
		throw Exception("Can't get magic", -errno);
	}

	LOG(mLog, DEBUG) << "Get magic: " << magic;

	return magic;
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

	if (mPollFd)
	{
		mPollFd->stop();
	}

	if (mThread.joinable())
	{
		mThread.join();
	}
}

DisplayItf::ConnectorPtr Display::createConnector(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mConnectorIds.find(name);

	if (it == mConnectorIds.end())
	{
		throw Exception("Can't create connector: " + name, -EINVAL);
	}

	return DisplayItf::ConnectorPtr(new Connector(name, mFd, it->second));
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

#ifdef WITH_ZCOPY
	if (isZeroCopySupported())
	{
		LOG(mLog, DEBUG) << "Create display buffer with zero copy";

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
#endif
	{
		LOG(mLog, DEBUG) << "Create display buffer";

		if (allocRefs)
		{
			throw  Exception("Can't allocate refs: ZCopy disabled", -EINVAL);
		}

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
		throw Exception("Cannot open DRM device: " + mName, -errno);
	}

	mPollFd.reset(new PollFd(mFd, POLLIN));

	uint64_t hasDumb = false;

	if (drmGetCap(mFd, DRM_CAP_DUMB_BUFFER, &hasDumb) < 0 || !hasDumb)
	{
		throw Exception("Drm device does not support dumb buffers", -errno);
	}

#ifdef WITH_ZCOPY
	mZeroCopyFd = drmOpen(XENDRM_ZCOPY_DRIVER_NAME, NULL);

	if (mZeroCopyFd < 0)
	{
		LOG(mLog, WARNING) << "Can't open zero copy driver. "
						   << "Zero copy functionality will be disabled.";
	}
#endif

	getConnectorIds();

	LOG(mLog, DEBUG) << "Create Drm card: " << mName << ", FD: " << mFd
					 << ", ZCopyFD: " << mZeroCopyFd;
}

void Display::release()
{
	if (mZeroCopyFd >= 0)
	{
		drmClose(mZeroCopyFd);
	}

	if (mFd >= 0)
	{
		drmClose(mFd);
	}
}

void Display::getConnectorIds()
{
	ModeResource resource(mFd);

	for (int i = 0; i < resource->count_connectors; i++)
	{
		ModeConnector connector(mFd, resource->connectors[i]);

		string name = sConnectorNames.at(DRM_MODE_CONNECTOR_Unknown) + "-" +
				to_string(connector->connector_type_id);

		auto it = sConnectorNames.find(connector->connector_type);

		if (it != sConnectorNames.end())
		{
			name = it->second  + "-" + to_string(connector->connector_type_id);
		}

		mConnectorIds[name] = connector->connector_id;

		LOG(mLog, DEBUG) << "Connector id: " << connector->connector_id
						 << ", name: " << name
						 << ", connected: "
						 << (connector->connection == DRM_MODE_CONNECTED);
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
