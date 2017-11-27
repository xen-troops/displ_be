/*
 *  Kms class
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

#include "WaylandKms.hpp"

#include "KmsBuffer.hpp"
#include "Exception.hpp"

using std::lock_guard;
using std::mutex;
using std::string;

using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;
using DisplayItf::GrantRefs;

namespace Wayland {

/*******************************************************************************
 * Kms
 ******************************************************************************/

WaylandKms::WaylandKms(wl_registry* registry,
					   uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlKms(nullptr),
	mIsAuthenticated(false),
	mLog("WaylandKms")
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

WaylandKms::~WaylandKms()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

bool WaylandKms::isZeroCopySupported()
{
	lock_guard<mutex> lock(mMutex);

	if (mDrmDevice && mDrmDevice->isZeroCopySupported() && mIsAuthenticated)
	{
		return true;
	}

	return false;
}

DisplayBufferPtr
WaylandKms::createDumb(uint32_t width, uint32_t height, uint32_t bpp,
					   domid_t domId, GrantRefs& refs, bool allocRefs)
{
	lock_guard<mutex> lock(mMutex);

	if (mDrmDevice && mDrmDevice->isZeroCopySupported())
	{
		return mDrmDevice->createDisplayBuffer(width, height, bpp,
											   domId, refs, allocRefs);
	}

	throw Exception("Can't create dumb: no DRM device", -EINVAL);
}

FrameBufferPtr
WaylandKms::createKmsBuffer(DisplayBufferPtr displayBuffer,
							uint32_t width,uint32_t height,
							uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	return  FrameBufferPtr(new KmsBuffer(mWlKms, displayBuffer, width,
										 height, pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void WaylandKms::sOnDevice(void *data, wl_kms *kms, const char *name)
{
	static_cast<WaylandKms*>(data)->onDevice(name);
}

void WaylandKms::sOnFormat(void *data, wl_kms *kms, uint32_t format)
{
	static_cast<WaylandKms*>(data)->onFormat(format);
}

void WaylandKms::sOnAuthenticated(void *data, wl_kms *kms)
{
	static_cast<WaylandKms*>(data)->onAuthenticated();
}

void WaylandKms::onDevice(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onDevice name: " << name;

	mDrmDevice.reset(new Drm::Display(name));

	if (mDrmDevice->isZeroCopySupported())
	{
		wl_kms_authenticate(mWlKms, mDrmDevice->getMagic());
	}
}

void WaylandKms::onFormat(uint32_t format)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onFormat format: 0x" << std::hex << format;
}

void WaylandKms::onAuthenticated()
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "onAuthenticated";

	mIsAuthenticated = true;
}

void WaylandKms::init()
{
	mWlKms = static_cast<wl_kms*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_kms_interface, getVersion()));

	if (!mWlKms)
	{
		throw Exception("Can't bind kms", -EINVAL);
	}

	mWlListener = {sOnDevice, sOnFormat, sOnAuthenticated};

	if (wl_kms_add_listener(mWlKms, &mWlListener, this) < 0)
	{
		throw Exception("Can't add listener", -EINVAL);
	}

	LOG(mLog, DEBUG) << "Create";
}

void WaylandKms::release()
{
	if (mWlKms)
	{
		wl_kms_destroy(mWlKms);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
