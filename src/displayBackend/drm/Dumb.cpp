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
#include <xen/be/XenGnttab.hpp>
#endif

#include "Exception.hpp"

using std::string;

using XenBackend::XenGnttabBuffer;
using XenBackend::XenGnttabDmaBufferImporter;

using DisplayItf::GrantRefs;

namespace Drm {

/*******************************************************************************
 * DumbBase
 ******************************************************************************/

DumbBase::DumbBase(int drmFd, uint32_t width, uint32_t height) :
	mDrmFd(drmFd),
	mBufDrmHandle(0),
	mBackStride(0),
	mFrontStride(0),
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
			throw Exception("Cannot get name", errno);
		}

		mName = req.name;
	}

	return mName;
}

void DumbBase::copy()
{
	throw Exception("There is no buffer to copy from", EINVAL);
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void DumbBase::createDumb(uint32_t bpp)
{
	drm_mode_create_dumb creq {0};

	mFrontStride = 4 * ((mWidth * bpp + 31) / 32);

	creq.width = mWidth;
	creq.height = mHeight;
	creq.bpp = bpp;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0)
	{
		throw Exception("Cannot create dumb buffer", errno);
	}

	mBackStride = creq.pitch;
	mSize = creq.size;
	mBufDrmHandle = creq.handle;
	if (mBackStride != mFrontStride)
	{
		DLOG(mLog, WARNING) << "Strides are different, frontend stride: " << mFrontStride << ", backend stride: " << mBackStride;
	}
}

/*******************************************************************************
 * DumbDrm
 ******************************************************************************/

DumbDrm::DumbDrm(int drmFd, uint32_t width, uint32_t height, uint32_t bpp,
				 domid_t domId, const GrantRefs& refs) :
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
		throw Exception("There is no buffer to copy from", EINVAL);
	}

	DLOG(mLog, DEBUG) << "Copy dumb, handle: " << mBufDrmHandle;
	if (mGnttabBuffer->size() == mSize)
	{
		memcpy(mBuffer, mGnttabBuffer->get(), mSize);
		return;
	}
	auto src = reinterpret_cast<uint8_t*>(mGnttabBuffer->get());
	auto dst = reinterpret_cast<uint8_t*>(mBuffer);
	for (unsigned int i = 0; i < mHeight; i++)
	{
		memcpy(dst + i * mBackStride, src + i * mFrontStride, mFrontStride);
	}
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
		throw Exception("Cannot map dumb buffer", errno);
	}

	auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					mDrmFd, mreq.offset);

	if (map == MAP_FAILED)
	{
		throw Exception("Cannot mmap dumb buffer", errno);
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
					   << mSize << ", stride: " << mBackStride;
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


#ifdef WITH_ZCOPY

/*******************************************************************************
 * DumbZCopyFront
 ******************************************************************************/

DumbZCopyFront::DumbZCopyFront(int drmFd,
							   uint32_t width, uint32_t height, uint32_t bpp,
							   domid_t domId, const GrantRefs& refs) :
	DumbBase(drmFd, width, height),
	mGnttabBuffer(domId, refs)
{
	mBufZCopyFd = mGnttabBuffer.getFd();
	mBackStride = 4 * ((width * bpp + 31) / 32);
	DLOG(mLog, DEBUG) << "Fd: " << mBufZCopyFd;
}

DumbZCopyFront::~DumbZCopyFront()
{
	DLOG(mLog, DEBUG) << "Will delete ZCopy front dumb"
					  << ", fd: " << mBufZCopyFd;

	int ret = mGnttabBuffer.waitForReleased(cBufZCopyWaitHandleToMs);
	if (ret < 0)
	{
		ret = errno;
	}

	if (ret && ret != ENOENT)
	{
		DLOG(mLog, ERROR) << "Wait for buffer failed, force releasing"
						  << ", error: " << strerror(ret)
						  << ", fd: " << mBufZCopyFd;
	}

	DLOG(mLog, DEBUG) << "Delete ZCopy front dumb"
					  << ", fd: " << mBufZCopyFd;
}

/*******************************************************************************
 * DumbZCopyFrontDrm
 ******************************************************************************/

DumbZCopyFrontDrm::DumbZCopyFrontDrm(int drmFd,
									 uint32_t width, uint32_t height,
									 uint32_t bpp, domid_t domId,
									 const GrantRefs& refs) :
	DumbZCopyFront(drmFd, width, height, bpp, domId, refs)
{
	drm_prime_handle prime {0};

	prime.fd = mBufZCopyFd;
	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0)
	{
		throw Exception("Cannot import prime buffer", errno);
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

DumbZCopyBack::DumbZCopyBack(int drmFd,
							 uint32_t width, uint32_t height, uint32_t bpp,
							 domid_t domId, GrantRefs& refs) :
	DumbBase(drmFd, width, height),
	mBufDrmFd(-1)
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

void DumbZCopyBack::getBufDrmFd()
{
	drm_prime_handle prime {0};

	prime.handle = mBufDrmHandle;

	prime.flags = DRM_CLOEXEC;

	if (drmIoctl(mDrmFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) < 0)
	{
		throw Exception("Cannot export prime buffer", errno);
	}

	mBufDrmFd = prime.fd;
}

void DumbZCopyBack::getGrantRefs(domid_t domId, GrantRefs& refs)
{
	refs.assign((mSize + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE, 0);

	mGnttabBuffer.reset(new XenGnttabDmaBufferImporter(domId, mBufDrmFd, refs));
}

void DumbZCopyBack::init(uint32_t bpp, domid_t domId, GrantRefs& refs)
{
	createDumb(bpp);
	getBufDrmFd();
	getGrantRefs(domId, refs);

	DLOG(mLog, DEBUG) << "Create ZCopy back dumb, domId: " << domId
					  << ", handle: " << mBufDrmHandle
					  << ", fd: " << mBufDrmFd
					  << ", size: " << mSize << ", stride: " << mBackStride;
}

void DumbZCopyBack::release()
{
	mGnttabBuffer.reset();

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

#endif

}
