/*
 *  Dumb class
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

#include "Dumb.hpp"

#include <sys/mman.h>

#include <xen/be/Log.hpp>

#include "Device.hpp"

using std::exception;

namespace Drm {

/*******************************************************************************
 * Dumb
 ******************************************************************************/

Dumb::Dumb(Device& drm, uint32_t width, uint32_t height, uint32_t bpp) :
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

}

