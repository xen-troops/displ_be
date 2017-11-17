/*
 * DumbZCopyBack.cpp
 *
 *  Created on: Jan 20, 2017
 *      Author: al1
 */

#include "DumbZCopyBack.hpp"

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>

#include <drm/xen_zcopy_drm.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using DisplayItf::GrantRefs;

namespace Drm {

/*******************************************************************************
 * DumbZCopyBack
 ******************************************************************************/

DumbZCopyBack::DumbZCopyBack(int drmFd, int zeroCopyFd,
							 uint32_t width, uint32_t height, uint32_t bpp,
							 domid_t domId, GrantRefs& refs) :
	mDrmFd(drmFd),
	mZeroCopyFd(zeroCopyFd),
	mHandle(0),
	mMappedHandle(0),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mName(0),
	mSize(0),
	mLog("DumbZCopyBack")
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

DumbZCopyBack::~DumbZCopyBack()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint32_t DumbZCopyBack::readName()
{
	if (!mName)
	{
		drm_gem_flink req = { .handle = mHandle };

		if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_FLINK, &req) < 0)
		{
			throw Exception("Cannot get name");
		}

		mName = req.name;
	}

	return mName;
}

void DumbZCopyBack::copy()
{
	throw Exception("There is no buffer to copy from");
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DumbZCopyBack::createDumb(uint32_t bpp)
{
	drm_mode_create_dumb creq {0};

	creq.width = mWidth;
	creq.height = mHeight;
	creq.bpp = bpp;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0)
	{
		throw Exception("Cannot create dumb buffer");
	}

	mStride = creq.pitch;
	mSize = creq.size;
	mHandle = creq.handle;
}

void DumbZCopyBack::createHandle()
{
	drm_prime_handle prime {0};

	prime.handle = mHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw Exception("Cannot export prime buffer.");
	}

	mHandleFd = prime.fd;
	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mZeroCopyFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw Exception("Cannot import prime buffer.");
	}

	mMappedHandle = prime.handle;
}

void DumbZCopyBack::getGrantRefs(domid_t domId, GrantRefs& refs)
{
	drm_xen_zcopy_dumb_to_refs mapreq {0};

	refs.resize((mSize + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE);

	mapreq.otherend_id = domId;
	mapreq.handle = mMappedHandle;
	mapreq.grefs = refs.data();
	mapreq.num_grefs = refs.size();

	if (drmIoctl(mZeroCopyFd, DRM_IOCTL_XEN_ZCOPY_DUMB_TO_REFS, &mapreq) < 0)
	{
		throw Exception("Cannot convert dumb buffer to refs");
	}
}

void DumbZCopyBack::init(uint32_t bpp, domid_t domId, GrantRefs& refs)
{
	createDumb(bpp);
	createHandle();
	getGrantRefs(domId, refs);

	DLOG(mLog, DEBUG) << "Create dumb, domId: " << domId
					  << ", handle: " << mHandle
					  << ", size: " << mSize << ", stride: " << mStride;
}

void DumbZCopyBack::release()
{
	if (mMappedHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mMappedHandle;

		drmIoctl(mZeroCopyFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}

	if (mHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mHandle;

		drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &closeReq);

		close(mHandleFd);
	}

	DLOG(mLog, DEBUG) << "Delete dumb, handle: " << mHandle;
}

}
