/*
 * Dumb.cpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#include "DumbZeroCopy.hpp"

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>

#include <drm/xen_zcopy_drm.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using std::exception;

namespace Drm {

/*******************************************************************************
 * DumbZeroCopy
 ******************************************************************************/

DumbZeroCopy::DumbZeroCopy(int mapFd, int drmFd,
						   uint32_t width, uint32_t height, uint32_t bpp,
						   domid_t domId, const vector<grant_ref_t>& refs) :
	mDrmFd(drmFd),
	mMappedFd(mapFd),
	mHandle(0),
	mMappedHandle(0),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mName(0),
	mSize(0),
	mBuffer(nullptr),
	mLog("DumbZeroCopy")
{
	try
	{
		init(bpp, domId, refs);
	}
	catch(const exception& e)
	{
		release();

		throw;
	}
}

DumbZeroCopy::~DumbZeroCopy()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint32_t DumbZeroCopy::readName()
{
	if (!mName)
	{
		drm_gem_flink req = { .handle = mHandle };

		if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_FLINK, &req) < 0)
		{
			throw DrmException("Cannot get name");
		}

		mName = req.name;
	}

	return mName;
}

void DumbZeroCopy::copy()
{
	throw DrmException("There is no buffer to copy from");
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DumbZeroCopy::createDumb(uint32_t bpp, domid_t domId,
							  const vector<grant_ref_t>& refs)
{
	drm_xen_zcopy_create_dumb mapreq {0};

	mapreq.otherend_id = domId;
	mapreq.grefs = const_cast<grant_ref_t*>(refs.data());
	mapreq.num_grefs = refs.size();

	mapreq.dumb.width = mWidth;
	mapreq.dumb.height = mHeight;
	mapreq.dumb.bpp = bpp;

	if (drmIoctl(mMappedFd, DRM_IOCTL_XEN_ZCOPY_CREATE_DUMB, &mapreq) < 0)
	{
		throw DrmException("Cannot create mapped dumb buffer");
	}

	mStride = mapreq.dumb.pitch;
	mSize = mapreq.dumb.size;
	mMappedHandle = mapreq.dumb.handle;
}

void DumbZeroCopy::createHandle()
{
	drm_prime_handle prime {0};

	prime.handle = mMappedHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mMappedFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw DrmException("Cannot export prime buffer.");
	}

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw DrmException("Cannot import prime buffer.");
	}

	mHandle = prime.handle;
}

void DumbZeroCopy::mapDumb()
{
	drm_mode_map_dumb mreq {0};

	mreq.handle = mHandle;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0)
	{
		throw DrmException("Cannot map dumb buffer.");
	}

	auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					mDrmFd, mreq.offset);

	if (map == MAP_FAILED)
	{
		throw DrmException("Cannot mmap dumb buffer");
	}

	mBuffer = map;
}

void DumbZeroCopy::init(uint32_t bpp, domid_t domId,
						const vector<grant_ref_t>& refs)
{

	createDumb(bpp, domId, refs);
	createHandle();
	mapDumb();

	DLOG(mLog, DEBUG) << "Create dumb, handle: " << mHandle << ", size: "
					  << mSize << ", stride: " << mStride;
}

void DumbZeroCopy::release()
{
	if (mBuffer)
	{
		munmap(mBuffer, mSize);
	}

	if (mHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mHandle;

		drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}
	if (mMappedHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mMappedHandle;

		drmIoctl(mMappedFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}

	DLOG(mLog, DEBUG) << "Delete dumb, handle: " << mHandle;
}

}
