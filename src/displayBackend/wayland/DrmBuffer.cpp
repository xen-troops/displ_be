/*
 *  Drm buffer class
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

#include "DrmBuffer.hpp"

#include "Exception.hpp"

using DisplayItf::DisplayBufferPtr;

namespace Wayland {

/*******************************************************************************
 * DrmBuffer
 ******************************************************************************/

DrmBuffer::DrmBuffer(wl_drm* wlDrm,
					 DisplayBufferPtr displayBuffer,
					 uint32_t width, uint32_t height,
					 uint32_t pixelFormat) :
	mDisplayBuffer(displayBuffer),
	mWlBuffer(nullptr),
	mWidth(width),
	mHeight(height),
	mLog("DrmBuffer")
{
	try
	{
		init(wlDrm, pixelFormat);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

DrmBuffer::~DrmBuffer()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void DrmBuffer::init(wl_drm* wlDrm, uint32_t pixelFormat)
{
	mWlBuffer = wl_drm_create_buffer(wlDrm, mDisplayBuffer->readName(),
									 mWidth, mHeight,
									 mDisplayBuffer->getStride(), pixelFormat);

	if (!mWlBuffer)
	{
		throw Exception("Can't create DRM buffer", -EINVAL);
	}

	LOG(mLog, DEBUG) << "Create, name: " << mDisplayBuffer->readName()
					 << ", w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mDisplayBuffer->getStride()
					 << ", format: " << pixelFormat;
}

void DrmBuffer::release()
{
	if (mWlBuffer)
	{
		wl_buffer_destroy(mWlBuffer);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}

