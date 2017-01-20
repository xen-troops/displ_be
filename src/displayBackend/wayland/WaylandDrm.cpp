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

#include "WaylandDrm.hpp"

#include "DrmBuffer.hpp"
#include "Exception.hpp"

using std::lock_guard;
using std::mutex;
using std::string;

using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;
using DisplayItf::GrantRefs;

namespace Wayland {

/*******************************************************************************
 * Drm
 ******************************************************************************/

WaylandDrm::WaylandDrm(wl_registry* registry,
					   uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlDrm(nullptr),
	mIsAuthenticated(false),
	mLog("WaylandDrm")
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

WaylandDrm::~WaylandDrm()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

bool WaylandDrm::isZeroCopySupported()
{
	lock_guard<mutex> lock(mMutex);

	if (mDrmDevice && mDrmDevice->isZeroCopySupported() && mIsAuthenticated)
	{
		return true;
	}

	return false;
}

DisplayBufferPtr
WaylandDrm::createDumb(uint32_t width, uint32_t height, uint32_t bpp,
					   domid_t domId, GrantRefs& refs)
{
	lock_guard<mutex> lock(mMutex);

	if (mDrmDevice)
	{
		return mDrmDevice->createDisplayBuffer(width, height, bpp, domId, refs);
	}

	throw Exception("Can't create dumb: no DRM device");
}

FrameBufferPtr
WaylandDrm::createDrmBuffer(DisplayBufferPtr displayBuffer,
							uint32_t width,uint32_t height,
							uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	return  FrameBufferPtr(new DrmBuffer(mWlDrm, displayBuffer, width,
												height, pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void WaylandDrm::sOnDevice(void *data, wl_drm *drm, const char *name)
{
	static_cast<WaylandDrm*>(data)->onDevice(name);
}

void WaylandDrm::sOnFormat(void *data, wl_drm *drm, uint32_t format)
{
	static_cast<WaylandDrm*>(data)->onFormat(format);
}

void WaylandDrm::sOnAuthenticated(void *data, wl_drm *drm)
{
	static_cast<WaylandDrm*>(data)->onAuthenticated();
}

void WaylandDrm::sOnCapabilities(void *data, wl_drm *drm, uint32_t value)
{
	static_cast<WaylandDrm*>(data)->onCapabilities(value);
}

void WaylandDrm::onDevice(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onDevice name: " << name;

	mDrmDevice.reset(new Drm::Display(name));

	wl_drm_authenticate(mWlDrm, mDrmDevice->getMagic());
}

void WaylandDrm::onFormat(uint32_t format)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onFormat format: " << format;
}

void WaylandDrm::onAuthenticated()
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onAuthenticated";

	mIsAuthenticated = true;
}

void WaylandDrm::onCapabilities(uint32_t value)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onCapabilities value: " << value;
}

void WaylandDrm::init()
{
	mWlDrm = static_cast<wl_drm*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_drm_interface, getVersion()));

	if (!mWlDrm)
	{
		throw Exception("Can't bind drm");
	}

	mWlListener = {sOnDevice, sOnFormat, sOnAuthenticated, sOnCapabilities};

	if (wl_drm_add_listener(mWlDrm, &mWlListener, this) < 0)
	{
		throw Exception("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void WaylandDrm::release()
{
	if (mWlDrm)
	{
		wl_drm_destroy(mWlDrm);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
