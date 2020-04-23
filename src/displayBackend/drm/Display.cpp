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
#include "DrmDeviceDetector.hpp"

#include <fcntl.h>
#include <signal.h>

#include <xf86drm.h>

#include <xen/be/Log.hpp>

#include "Dumb.hpp"

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
 * Display
 ******************************************************************************/
Display::Display(const string& name) :
	mDrmFd(-1),
	mLog("Drm"),
	mName(name),
	mStarted(false)
{
	if (name.empty())
	{
		mName = detectDrmDevice();
	}

	mDrmFd = open(mName.c_str(), O_RDWR | O_CLOEXEC);

	if (mDrmFd < 0)
	{
		throw Exception("Cannot open DRM device: " + mName, errno);
	}

	LOG(mLog, DEBUG) << "Create Drm card: " << mName << ", FD: " << mDrmFd;

	mPollFd.reset(new PollFd(mDrmFd, POLLIN));

	uint64_t hasDumb = false;

	if (drmGetCap(mDrmFd, DRM_CAP_DUMB_BUFFER, &hasDumb) < 0 || !hasDumb)
	{
		throw Exception("Drm device does not support dumb buffers", errno);
	}

	getConnectorIds();
}

Display::~Display()
{
	stop();

	if (mDrmFd >= 0)
	{
		drmClose(mDrmFd);
	}

	LOG(mLog, DEBUG) << "Delete Drm card: " << mName;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

drm_magic_t Display::getMagic()
{
	lock_guard<mutex> lock(mMutex);

	drm_magic_t magic = 0;

	if (drmGetMagic(mDrmFd, &magic) < 0)
	{
		throw Exception("Can't get magic", errno);
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

	mStarted = true;

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

	mStarted = false;
}

void Display::flush()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "flush";
}

DisplayItf::ConnectorPtr Display::createConnector(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mConnectorIds.find(name);

	if (it == mConnectorIds.end())
	{
		throw Exception("Can't create connector: " + name, EINVAL);
	}

	return DisplayItf::ConnectorPtr(new Connector(name, mDrmFd, it->second));
}

DisplayBufferPtr Display::createDisplayBuffer(uint32_t width, uint32_t height,
											 uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create display buffer";

	return DisplayBufferPtr(new DumbDrm(mDrmFd, width, height, bpp));
}

DisplayBufferPtr Display::createDisplayBuffer(
		uint32_t width, uint32_t height, uint32_t bpp,
		domid_t domId, DisplayItf::GrantRefs& refs, bool allocRefs)
{
	lock_guard<mutex> lock(mMutex);

#ifdef WITH_ZCOPY
	LOG(mLog, DEBUG) << "Create display buffer with zero copy";

	if (allocRefs)
	{
		return DisplayBufferPtr(new DumbZCopyBack(mDrmFd,
												  width, height, bpp,
												  domId, refs));
	}

	return DisplayBufferPtr(new DumbZCopyFrontDrm(mDrmFd,
												  width, height, bpp,
												  domId, refs));
#else
	LOG(mLog, DEBUG) << "Create display buffer";

	if (allocRefs)
	{
		throw  Exception("Can't allocate refs: ZCopy disabled", EINVAL);
	}

	return DisplayBufferPtr(new DumbDrm(mDrmFd, width, height, bpp,
										domId, refs));
#endif
}

FrameBufferPtr Display::createFrameBuffer(DisplayBufferPtr displayBuffer,
										 uint32_t width, uint32_t height,
										 uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create frame buffer";

	return FrameBufferPtr(new FrameBuffer(mDrmFd, displayBuffer, width,
										  height, pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Display::getConnectorIds()
{
	ModeResource resource(mDrmFd);

	for (int i = 0; i < resource->count_connectors; i++)
	{
		ModeConnector connector(mDrmFd, resource->connectors[i]);

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
			drmHandleEvent(mDrmFd, &ev);
		}
	}
	catch(const std::exception& e)
	{
		LOG(mLog, ERROR) << e.what();

		kill(getpid(), SIGTERM);
	}
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

#if defined(WITH_WAYLAND) && defined(WITH_ZCOPY)
DisplayBufferPtr DisplayWayland::createDisplayBuffer(
		uint32_t width, uint32_t height, uint32_t bpp,
		domid_t domId, GrantRefs& refs, bool allocRefs)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create display buffer with zero copy";

	if (allocRefs)
	{
		return DisplayBufferPtr(new DumbZCopyBack(mDrmFd,
												  width, height, bpp,
												  domId, refs));
	}

	return DisplayBufferPtr(new DumbZCopyFront(mDrmFd,
											   width, height, bpp,
											   domId, refs));
}
#endif

}
