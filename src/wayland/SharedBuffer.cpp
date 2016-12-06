/*
 * SharedBuffer.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: al1
 */

#include "SharedBuffer.hpp"

#include <drm_fourcc.h>

#include "Exception.hpp"

using std::shared_ptr;

namespace Wayland {

/*******************************************************************************
 * SharedBuffer
 ******************************************************************************/
SharedBuffer::SharedBuffer(wl_shm* sharedMemory,
						   shared_ptr<SharedFile> sharedFile,
						   uint32_t width, uint32_t height,
						   uint32_t pixelFormat) :
	mSharedFile(sharedFile),
	mBuffer(nullptr),
	mPool(nullptr),
	mWidth(width),
	mHeight(height),
	mLog("SharedBuffer")
{
	try
	{
		init(sharedMemory, pixelFormat);
	}
	catch(const WlException& e)
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

void SharedBuffer::init(wl_shm* sharedMemory, uint32_t pixelFormat)
{
	mPool = wl_shm_create_pool(sharedMemory, mSharedFile->mFd,
							   mHeight * mSharedFile->mStride);

	if (!mPool)
	{
		throw WlException("Can't create pool");
	}

	mBuffer = wl_shm_pool_create_buffer(mPool, 0, mWidth, mHeight,
										mSharedFile->mStride,
										convertPixelFormat(pixelFormat));

	if (!mBuffer)
	{
		throw WlException("Can't create shared buffer");
	}

	wl_shm_pool_destroy(mPool);

	mPool = nullptr;

	LOG(mLog, DEBUG) << "Create, w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mSharedFile->mStride
					 << ", fd: " << mSharedFile->mFd
					 << ", format: " << pixelFormat;
}

void SharedBuffer::release()
{
	if (mPool)
	{
		wl_shm_pool_destroy(mPool);
	}

	if (mBuffer)
	{
		wl_buffer_destroy(mBuffer);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
