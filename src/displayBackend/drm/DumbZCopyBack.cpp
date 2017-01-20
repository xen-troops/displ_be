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

DumbZCopyBack::DumbZCopyBack(int mapFd, int drmFd,
							 uint32_t width, uint32_t height, uint32_t bpp,
							  domid_t domId, GrantRefs& refs) :
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

void DumbZCopyBack::mapDumb()
{
	drm_mode_map_dumb mreq {0};

	mreq.handle = mHandle;

	if (drmIoctl(mDrmFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0)
	{
		throw Exception("Cannot map dumb buffer.");
	}

	auto map = mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					mDrmFd, mreq.offset);

	if (map == MAP_FAILED)
	{
		throw Exception("Cannot mmap dumb buffer");
	}

	mBuffer = map;
}

void DumbZCopyBack::getGrantRefs(domid_t domId, DisplayItf::GrantRefs& refs)
{
	// Put here getting refs
	// refs.assign() or refs.push_back();
}

void DumbZCopyBack::init(uint32_t bpp, domid_t domId,  GrantRefs& refs)
{

	createDumb(bpp);
	mapDumb();
	getGrantRefs(domId, refs);

	DLOG(mLog, DEBUG) << "Create dumb, handle: " << mHandle << ", size: "
					  << mSize << ", stride: " << mStride;
}

void DumbZCopyBack::release()
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
