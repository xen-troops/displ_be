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

#include <xf86drm.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using std::string;
using std::vector;

using XenBackend::XenGnttabBuffer;

namespace Drm {

/*******************************************************************************
 * Dumb
 ******************************************************************************/

Dumb::Dumb(int fd, uint32_t width, uint32_t height, uint32_t bpp,
		   domid_t domId, const vector<grant_ref_t>& refs) :
	mFd(fd),
	mHandle(cInvalidId),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mName(0),
	mSize(0),
	mBuffer(nullptr),
	mLog("Dumb")
{
	try
	{
		init(bpp, domId, refs);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

Dumb::~Dumb()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint32_t Dumb::readName()
{
	if (!mName)
	{
		drm_gem_flink req = { .handle = mHandle };

		if (drmIoctl(mFd, DRM_IOCTL_GEM_FLINK, &req) < 0)
		{
			throw Exception("Cannot get name");
		}

		mName = req.name;
	}

	return mName;
}

void Dumb::copy()
{
	if(!mGnttabBuffer)
	{
		throw Exception("There is no buffer to copy from");
	}

	DLOG(mLog, DEBUG) << "Copy dumb, handle: " << mHandle;

	memcpy(mBuffer, mGnttabBuffer->get(), mSize);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Dumb::createDumb(uint32_t bpp)
{
	drm_mode_create_dumb creq {0};

	creq.width = mWidth;
	creq.height = mHeight;
	creq.bpp = bpp;

	if (drmIoctl(mFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0)
	{
		throw Exception("Cannot create dumb buffer");
	}

	mStride = creq.pitch;
	mSize = creq.size;
	mHandle = creq.handle;
}

void Dumb::mapDumb()
{
	drm_mode_map_dumb mreq {0};

	mreq.handle = mHandle;

	if (drmIoctl(mFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0)
	{
		throw Exception("Cannot map dumb buffer.");
	}

	auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					mFd, mreq.offset);

	if (map == MAP_FAILED)
	{
		throw Exception("Cannot mmap dumb buffer");
	}

	mBuffer = map;
}

void Dumb::init(uint32_t bpp, domid_t domId, const vector<grant_ref_t>& refs)
{
	if (refs.size())
	{
		mGnttabBuffer.reset(
				new XenGnttabBuffer(domId, refs.data(), refs.size()));
	}

	createDumb(bpp);
	mapDumb();

	DLOG(mLog, DEBUG) << "Create dumb, handle: " << mHandle << ", size: "
					   << mSize << ", stride: " << mStride;
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

		drmIoctl(mFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	}

	DLOG(mLog, DEBUG) << "Delete dumb, handle: " << mHandle;
}

}

