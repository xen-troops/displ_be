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

#ifdef WITH_ZCOPY
#include <fcntl.h>
#include <drm/xen_zcopy_drm.h>
#endif

#include "Exception.hpp"

using std::string;

using XenBackend::XenGnttabBuffer;

using DisplayItf::GrantRefs;

namespace Drm {

/*******************************************************************************
 * DumbBase
 ******************************************************************************/

DumbBase::DumbBase(int drmFd, uint32_t width, uint32_t height) :
	mDrmFd(drmFd),
	mBufDrmHandle(0),
	mStride(0),
	mWidth(width),
	mHeight(height),
	mName(0),
	mSize(0),
	mLog("Dumb")
{
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint32_t DumbBase::readName()
{
	if (!mName && mBufDrmHandle)
	{
		drm_gem_flink req = { .handle = mBufDrmHandle };

		if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_FLINK, &req) < 0)
		{
			throw Exception("Cannot get name", -errno);
		}

		mName = req.name;
	}

	return mName;
}

void DumbBase::copy()
{
	throw Exception("There is no buffer to copy from", -EINVAL);
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void DumbBase::createDumb(uint32_t bpp)
{
	drm_mode_create_dumb creq {0};

	creq.width = mWidth;
	creq.height = mHeight;
	creq.bpp = bpp;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0)
	{
		throw Exception("Cannot create dumb buffer", -errno);
	}

	mStride = creq.pitch;
	mSize = creq.size;
	mBufDrmHandle = creq.handle;
}

/*******************************************************************************
 * DumbDrm
 ******************************************************************************/

DumbDrm::DumbDrm(int drmFd, uint32_t width, uint32_t height, uint32_t bpp,
				 domid_t domId, const DisplayItf::GrantRefs& refs) :
	DumbBase(drmFd, width, height),
	mBuffer(nullptr)
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

DumbDrm::~DumbDrm()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void DumbDrm::copy()
{
	if(!mGnttabBuffer)
	{
		throw Exception("There is no buffer to copy from", -EINVAL);
	}

	DLOG(mLog, DEBUG) << "Copy dumb, handle: " << mBufDrmHandle;

	memcpy(mBuffer, mGnttabBuffer->get(), mSize);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DumbDrm::mapDumb()
{
	drm_mode_map_dumb mreq {0};

	mreq.handle = mBufDrmHandle;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0)
	{
		throw Exception("Cannot map dumb buffer", -errno);
	}

	auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					mDrmFd, mreq.offset);

	if (map == MAP_FAILED)
	{
		throw Exception("Cannot mmap dumb buffer", -errno);
	}

	mBuffer = map;
}

void DumbDrm::init(uint32_t bpp, domid_t domId, const GrantRefs& refs)
{
	if (refs.size())
	{
		mGnttabBuffer.reset(
				new XenGnttabBuffer(domId, refs.data(), refs.size()));
	}

	createDumb(bpp);
	mapDumb();

	DLOG(mLog, DEBUG) << "Create dumb, handle: " << mBufDrmHandle << ", size: "
					   << mSize << ", stride: " << mStride;
}

void DumbDrm::release()
{
	if (mBuffer)
	{
		munmap(mBuffer, mSize);
	}

	if (mBufDrmHandle)
	{
		drm_mode_destroy_dumb dreq {0};

		dreq.handle = mBufDrmHandle;

		drmIoctl(mDrmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	}

	DLOG(mLog, DEBUG) << "Delete dumb, handle: " << mBufDrmHandle;
}

/*******************************************************************************
 * DumbZCopyFront
 ******************************************************************************/

DumbZCopyFront::DumbZCopyFront(int drmFd, int zCopyFd,
							   uint32_t width, uint32_t height, uint32_t bpp,
							   domid_t domId, const GrantRefs& refs) :
	DumbBase(drmFd, width, height),
	mZCopyFd(zCopyFd)
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

	if (drmIoctl(mZCopyFd, DRM_IOCTL_XEN_ZCOPY_DUMB_FROM_REFS, &mapreq) < 0)
	{
		throw Exception("Cannot create dumb", -errno);
	}

	mStride = mapreq.dumb.pitch;
	mSize = mapreq.dumb.size;
	mBufZCopyHandle = mapreq.dumb.handle;
	mBufZCopyWaitHandle = mapreq.wait_handle;
}

void DumbZCopyFront::getBufFd()
{
	drm_prime_handle prime {0};

	prime.handle = mBufZCopyHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mZCopyFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw Exception("Cannot export prime buffer", -errno);
	}

	mBufZCopyFd = prime.fd;
}

void DumbZCopyFront::init(uint32_t bpp, domid_t domId, const GrantRefs& refs)
{

	createDumb(bpp, domId, refs);
	getBufFd();

	DLOG(mLog, DEBUG) << "Create ZCopy front dumb, domId: " << domId
					  << ", handle: " << mBufZCopyHandle
					  << ", fd: " << mBufZCopyFd
					  << ", size: " << mSize << ", stride: " << mStride;
}

void DumbZCopyFront::release()
{
	if (mBufZCopyHandle != 0)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mBufZCopyHandle;

		drmIoctl(mZCopyFd, DRM_IOCTL_GEM_CLOSE, &closeReq);

		close(mBufZCopyFd);

		drm_xen_zcopy_dumb_wait_free waitReq {};

		waitReq.wait_handle = mBufZCopyWaitHandle;
		waitReq.wait_to_ms = cBufZCopyWaitHandleToMs;

		int ret = drmIoctl(mZCopyFd, DRM_IOCTL_XEN_ZCOPY_DUMB_WAIT_FREE, &waitReq);

		if ((ret < 0) && (errno != ENOENT))
		{
			DLOG(mLog, ERROR) << "Wait for buffer failed, force releasing"
							  << ", error: " << strerror(errno)
							  << ", handle: " << mBufZCopyHandle
							  << ", fd: " << mBufZCopyFd
							  << ", wait handle: " << mBufZCopyWaitHandle;
		}

		DLOG(mLog, DEBUG) << "Delete ZCopy front dumb"
						  << ", handle: " << mBufZCopyHandle
						  << ", fd: " << mBufZCopyFd
						  << ", wait handle: " << mBufZCopyWaitHandle;
	}
}

/*******************************************************************************
 * DumbZCopyFrontDrm
 ******************************************************************************/

DumbZCopyFrontDrm::DumbZCopyFrontDrm(int drmFd, int zCopyFd,
									 uint32_t width, uint32_t height,
									 uint32_t bpp, domid_t domId,
									 const GrantRefs& refs) :
	DumbZCopyFront(drmFd, zCopyFd, width, height, bpp, domId, refs)
{
	drm_prime_handle prime {0};

	prime.handle = mBufZCopyHandle;
	prime.fd = mBufZCopyFd;
	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw Exception("Cannot import prime buffer", -errno);
	}

	mBufDrmHandle = prime.handle;

	DLOG(mLog, DEBUG) << "Get ZCopy front dumb, handle: " << mBufDrmHandle;
}

DumbZCopyFrontDrm::~DumbZCopyFrontDrm()
{
	if (mBufDrmHandle)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mBufDrmHandle;

		drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &closeReq);

		DLOG(mLog, DEBUG) << "Close ZCopy front dumb, handle: "
						  << mBufDrmHandle;
	}
}

/*******************************************************************************
 * DumbZCopyBack
 ******************************************************************************/

DumbZCopyBack::DumbZCopyBack(int drmFd, int zCopyFd,
							 uint32_t width, uint32_t height, uint32_t bpp,
							 domid_t domId, GrantRefs& refs) :
	DumbBase(drmFd, width, height),
	mZCopyFd(zCopyFd)
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
 * Private
 ******************************************************************************/

void DumbZCopyBack::createHandle()
{
	drm_prime_handle prime {0};

	prime.handle = mBufDrmHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw Exception("Cannot export prime buffer", -errno);
	}

	mBufDrmFd = prime.fd;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mZCopyFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw Exception("Cannot import prime buffer", -errno);
	}

	mBufZCopyHandle = prime.handle;
}

void DumbZCopyBack::getGrantRefs(domid_t domId, GrantRefs& refs)
{
	drm_xen_zcopy_dumb_to_refs mapreq {0};

	refs.resize((mSize + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE);

	mapreq.otherend_id = domId;
	mapreq.handle = mBufZCopyHandle;
	mapreq.grefs = refs.data();
	mapreq.num_grefs = refs.size();

	if (drmIoctl(mZCopyFd, DRM_IOCTL_XEN_ZCOPY_DUMB_TO_REFS, &mapreq) < 0)
	{
		throw Exception("Cannot convert dumb buffer to refs", -errno);
	}
}

void DumbZCopyBack::init(uint32_t bpp, domid_t domId, GrantRefs& refs)
{
	createDumb(bpp);
	createHandle();
	getGrantRefs(domId, refs);

	DLOG(mLog, DEBUG) << "Create ZCopy back dumb, domId: " << domId
					  << ", handle: " << mBufDrmHandle
					  << ", fd: " << mBufDrmFd
					  << ", size: " << mSize << ", stride: " << mStride;
}

void DumbZCopyBack::release()
{
	if (mBufZCopyHandle)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mBufZCopyHandle;

		drmIoctl(mZCopyFd, DRM_IOCTL_GEM_CLOSE, &closeReq);
	}

	if (mBufDrmHandle)
	{
		drm_gem_close closeReq {};

		closeReq.handle = mBufDrmHandle;

		drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &closeReq);

		close(mBufDrmFd);

		DLOG(mLog, DEBUG) << "Delete ZCopy back dumb"
						  << ", handle: " << mBufDrmHandle
						  << ", fd: " << mBufDrmFd;
	}
}

}
