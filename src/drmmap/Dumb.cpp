/*
 * Dumb.cpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#include "Dumb.hpp"

#include <fcntl.h>

#include <xf86drm.h>

#include <sys/mman.h>

#include <drm/xen_zcopy_drm.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using std::exception;

Dumb::Dumb(int mapFd, int drmFd, int domId,
		   const std::vector<uint32_t>& refs,
		   uint32_t width, uint32_t height, uint32_t bpp) :
	mFd(drmFd),
	mMappedFd(mapFd),
	mHandle(0),
	mMappedHandle(0),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mSize(0),
	mBuffer(nullptr)
{
	try
	{
		drm_xen_zcopy_create_dumb mapreq {0};

		mapreq.otherend_id = domId;
		mapreq.grefs = const_cast<uint32_t*>(refs.data());
		mapreq.num_grefs = refs.size();

		mapreq.dumb.width = width;
		mapreq.dumb.height = height;
		mapreq.dumb.bpp = bpp;

		auto ret = drmIoctl(mapFd, DRM_IOCTL_XEN_ZCOPY_CREATE_DUMB, &mapreq);

		if (ret < 0)
		{
			throw DrmMapException("Cannot create mapped dumb buffer");
		}

		mStride = mapreq.dumb.pitch;
		mSize = mapreq.dumb.size;
		mMappedHandle = mapreq.dumb.handle;

		drm_prime_handle prime {0};

		prime.handle = mMappedHandle;

		prime.flags = DRM_CLOEXEC;

		ret = drmIoctl(mapFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime);

		if (ret < 0)
		{
			throw DrmMapException("Cannot export prime buffer.");
		}

		prime.flags = DRM_CLOEXEC;

		ret = drmIoctl(drmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime);

		if (ret < 0)
		{
			throw DrmMapException("Cannot import prime buffer.");
		}

		mHandle = prime.handle;

		drm_mode_map_dumb mreq {0};

		mreq.handle = mHandle;

		ret = drmIoctl(drmFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);

		if (ret < 0)
		{
			throw DrmMapException("Cannot map dumb buffer.");
		}

		auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
						mFd, mreq.offset);

		if (map == MAP_FAILED)
		{
			throw DrmMapException("Cannot mmap dumb buffer");
		}

		mBuffer = map;

		DLOG("Dumb", DEBUG) << "Create dumb, handle: " << mHandle << ", size: "
						   << mSize << ", stride: " << mStride;

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

	if (mHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mHandle;

		drmIoctl(mFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}
	if (mMappedHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mMappedHandle;

		drmIoctl(mMappedFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}
}
