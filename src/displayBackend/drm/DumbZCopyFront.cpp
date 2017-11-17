/*
 * Dumb.cpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#include "DumbZCopyFront.hpp"

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>

#include <drm/xen_zcopy_drm.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using DisplayItf::GrantRefs;

namespace Drm {

/*******************************************************************************
 * DumbZCopyFront
 ******************************************************************************/

DumbZCopyFront::DumbZCopyFront(int drmFd, int zeroCopyFd,
							   uint32_t width, uint32_t height, uint32_t bpp,
							   domid_t domId, const GrantRefs& refs) :
	mDrmFd(drmFd),
	mZeroCopyFd(zeroCopyFd),
	mHandle(0),
	mMappedHandle(0),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mName(0),
	mSize(0),
	mLog("DumbZCopyFront")
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

DumbZCopyFront::~DumbZCopyFront()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint32_t DumbZCopyFront::readName()
{
	if (!mName)
	{
		drm_gem_flink req = { .handle = mHandle };

		if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_FLINK, &req) < 0)
		{
			throw Exception("Cannot get name", -errno);
		}

		mName = req.name;
	}

	return mName;
}

void DumbZCopyFront::copy()
{
	throw Exception("There is no buffer to copy from", -EINVAL);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DumbZCopyFront::createDumb(uint32_t bpp, domid_t domId,
							  const GrantRefs& refs)
{
	drm_xen_zcopy_dumb_from_refs mapreq {0};

	mapreq.otherend_id = domId;
	mapreq.grefs = const_cast<grant_ref_t*>(refs.data());
	mapreq.num_grefs = refs.size();

	mapreq.dumb.width = mWidth;
	mapreq.dumb.height = mHeight;
	mapreq.dumb.bpp = bpp;

	if (drmIoctl(mZeroCopyFd, DRM_IOCTL_XEN_ZCOPY_DUMB_FROM_REFS, &mapreq) < 0)
	{
		throw Exception("Cannot create mapped dumb buffer", -errno);
	}

	mStride = mapreq.dumb.pitch;
	mSize = mapreq.dumb.size;
	mMappedHandle = mapreq.dumb.handle;
}

void DumbZCopyFront::createHandle()
{
	drm_prime_handle prime {0};

	prime.handle = mMappedHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mZeroCopyFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw Exception("Cannot export prime buffer", -errno);
	}

	mMappedHandleFd = prime.fd;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw Exception("Cannot import prime buffer", -errno);
	}

	mHandle = prime.handle;
}

void DumbZCopyFront::init(uint32_t bpp, domid_t domId, const GrantRefs& refs)
{

	createDumb(bpp, domId, refs);
	createHandle();

	DLOG(mLog, DEBUG) << "Create dumb, domId: " << domId
					  << ", handle: " << mHandle
					  << ", size: " << mSize << ", stride: " << mStride;
}

void DumbZCopyFront::release()
{
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

		drmIoctl(mZeroCopyFd, DRM_IOCTL_GEM_CLOSE, &closeReq);

		close(mMappedHandleFd);
	}

	DLOG(mLog, DEBUG) << "Delete dumb, handle: " << mHandle;
}

}
