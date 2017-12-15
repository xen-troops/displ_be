/*
 *  Frame buffers class
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

#include "Exception.hpp"
#include "FrameBuffer.hpp"

using std::hex;
using std::setfill;
using std::setw;

using DisplayItf::DisplayBufferPtr;

#ifndef DRM_FORMAT_ARGB8888
#define DRM_FORMAT_ARGB8888           0x34325241
#endif
#ifndef DRM_FORMAT_XRGB8888
#define DRM_FORMAT_XRGB8888           0x34325258
#endif

namespace Wayland {

/*******************************************************************************
 * WlBuffer
 ******************************************************************************/
WlBuffer::WlBuffer(DisplayBufferPtr displayBuffer,
				   uint32_t width, uint32_t height) :
	mDisplayBuffer(displayBuffer),
	mWidth(width),
	mHeight(height),
	mWlBuffer(nullptr),
	mLog("WlBuffer")
{
}

WlBuffer::~WlBuffer()
{
	if (mWlBuffer)
	{
		wl_buffer_destroy(mWlBuffer);

		LOG(mLog, DEBUG) << "Delete";
	}
}

/*******************************************************************************
 * Protected
 ******************************************************************************/
void WlBuffer::setListener()
{
	mWlListener = {sOnRelease};

	if (!mWlBuffer)
	{
		throw Exception("No wl buffer", ENOENT);
	}

	if (wl_buffer_add_listener(mWlBuffer, &mWlListener, this) < 0)
	{
		throw Exception("Can't add listener", errno);
	}
}

/*******************************************************************************
 * Private
 ******************************************************************************/
void WlBuffer::sOnRelease(void *data, wl_buffer *wlBuffer)
{
	static_cast<WlBuffer*>(data)->onRelease();
}

void WlBuffer::onRelease()
{
	LOG(mLog, DEBUG) << "Release";
}

/*******************************************************************************
 * SharedBuffer
 ******************************************************************************/
SharedBuffer::SharedBuffer(wl_shm* wlSharedMemory,
						   DisplayBufferPtr displayBuffer,
						   uint32_t width, uint32_t height,
						   uint32_t pixelFormat) :
	WlBuffer(displayBuffer, width, height),
	mWlPool(nullptr)
{
	try
	{
		init(wlSharedMemory, pixelFormat);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

SharedBuffer::~SharedBuffer()
{
	release();
}

uint32_t SharedBuffer::convertPixelFormat(uint32_t format)
{
	if (format == DRM_FORMAT_ARGB8888)
	{
		return WL_SHM_FORMAT_ARGB8888;
	}

	if (format == DRM_FORMAT_XRGB8888)
	{
		return WL_SHM_FORMAT_XRGB8888;
	}

	return format;
}

void SharedBuffer::init(wl_shm* wlSharedMemory, uint32_t pixelFormat)
{
	mWlPool = wl_shm_create_pool(wlSharedMemory, mDisplayBuffer->getFd(),
							   mHeight * mDisplayBuffer->getStride());

	if (!mWlPool)
	{
		throw Exception("Can't create pool", errno);
	}

	mWlBuffer = wl_shm_pool_create_buffer(mWlPool, 0, mWidth, mHeight,
										  mDisplayBuffer->getStride(),
										  convertPixelFormat(pixelFormat));

	if (!mWlBuffer)
	{
		throw Exception("Can't create shared buffer", errno);
	}

	setListener();

	wl_shm_pool_destroy(mWlPool);

	mWlPool = nullptr;

	LOG(mLog, DEBUG) << "Create shared buffer, w: " << mWidth
					 << ", h: " << mHeight
					 << ", stride: " << mDisplayBuffer->getStride()
					 << ", fd: " << mDisplayBuffer->getFd()
					 << ", format: 0x"  << hex << setfill('0') << setw(8)
					 << pixelFormat;
}

void SharedBuffer::release()
{
	if (mWlPool)
	{
		wl_shm_pool_destroy(mWlPool);
	}
}

#ifdef WITH_ZCOPY
/*******************************************************************************
 * KmsBuffer
 ******************************************************************************/
KmsBuffer::KmsBuffer(wl_kms* wlKms,
					 DisplayBufferPtr displayBuffer,
					 uint32_t width, uint32_t height,
					 uint32_t pixelFormat) :
	WlBuffer(displayBuffer, width, height)
{
	mWlBuffer = wl_kms_create_buffer(wlKms, mDisplayBuffer->getFd(),
									 mWidth, mHeight,
									 mDisplayBuffer->getStride(), pixelFormat,
									 mDisplayBuffer->getHandle());

	if (!mWlBuffer)
	{
		throw Exception("Can't create KMS buffer", errno);
	}

	setListener();

	LOG(mLog, DEBUG) << "Create KMS buffer, fd: " << mDisplayBuffer->getFd()
					 << ", w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mDisplayBuffer->getStride()
					 << ", format: " << pixelFormat;
}

/*******************************************************************************
 * DrmBuffer
 ******************************************************************************/

DrmBuffer::DrmBuffer(wl_drm* wlDrm,
					 DisplayBufferPtr displayBuffer,
					 uint32_t width, uint32_t height,
					 uint32_t pixelFormat) :
	WlBuffer(displayBuffer, width, height)
{
	mWlBuffer = wl_drm_create_buffer(wlDrm, mDisplayBuffer->readName(),
									 mWidth, mHeight,
									 mDisplayBuffer->getStride(), pixelFormat);

	if (!mWlBuffer)
	{
		throw Exception("Can't create DRM buffer", errno);
	}

	setListener();

	LOG(mLog, DEBUG) << "Create, name: " << mDisplayBuffer->readName()
					 << ", w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mDisplayBuffer->getStride()
					 << ", format: " << pixelFormat;
}

#endif

}
