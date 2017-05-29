/*
 * SharedBuffer.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: al1
 */

#include "SharedBuffer.hpp"

#include "Exception.hpp"

using DisplayItf::DisplayBufferPtr;

#ifndef DRM_FORMAT_ARGB8888
#define DRM_FORMAT_ARGB8888           0x34325241
#endif
#ifndef DRM_FORMAT_XRGB8888
#define DRM_FORMAT_XRGB8888           0x34325258
#endif

namespace Wayland {

/*******************************************************************************
 * SharedBuffer
 ******************************************************************************/
SharedBuffer::SharedBuffer(wl_shm* wlSharedMemory,
						   DisplayBufferPtr displayBuffer,
						   uint32_t width, uint32_t height,
						   uint32_t pixelFormat) :
	mDisplayBuffer(displayBuffer),
	mWlBuffer(nullptr),
	mWlPool(nullptr),
	mWidth(width),
	mHeight(height),
	mLog("SharedBuffer")
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

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

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
	mWlPool = wl_shm_create_pool(wlSharedMemory, mDisplayBuffer->getHandle(),
							   mHeight * mDisplayBuffer->getStride());

	if (!mWlPool)
	{
		throw Exception("Can't create pool");
	}

	mWlBuffer = wl_shm_pool_create_buffer(mWlPool, 0, mWidth, mHeight,
										  mDisplayBuffer->getStride(),
										  convertPixelFormat(pixelFormat));

	if (!mWlBuffer)
	{
		throw Exception("Can't create shared buffer");
	}

	wl_shm_pool_destroy(mWlPool);

	mWlPool = nullptr;

	LOG(mLog, DEBUG) << "Create, w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mDisplayBuffer->getStride()
					 << ", fd: " << mDisplayBuffer->getHandle()
					 << ", format: " << pixelFormat;
}

void SharedBuffer::release()
{
	if (mWlPool)
	{
		wl_shm_pool_destroy(mWlPool);
	}

	if (mWlBuffer)
	{
		wl_buffer_destroy(mWlBuffer);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
