/*
 * DrmMap.cpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#include "DrmMap.hpp"

#include <xf86drm.h>
#include <drm/xen_zcopy_drm.h>

#include "Exception.hpp"
#include "Dumb.hpp"

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::vector;

/*******************************************************************************
 * Display
 ******************************************************************************/

DrmMap::DrmMap(int drmFd) :
	mFd(-1),
	mDrmFd(drmFd),
	mLog("DrmMap")
{
	try
	{
		init();
	}
	catch(const DrmMapException& e)
	{
		release();

		throw;
	}
}

DrmMap::~DrmMap()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

shared_ptr<DisplayBufferItf> DrmMap::createDisplayBuffer(
		int domId, const vector<uint32_t>& refs,
		uint32_t width, uint32_t height, uint32_t bpp)
{
	LOG(mLog, DEBUG) << "Create display buffer";

	return shared_ptr<DisplayBufferItf>(new Dumb(mFd, mDrmFd, domId, refs,
												 width, height, bpp));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DrmMap::init()
{
	mFd = drmOpen(XENDRM_ZCOPY_DRIVER_NAME, NULL);

	if (mFd < 0)
	{
		throw DrmMapException("Can't open driver");
	}

	LOG(mLog, DEBUG) << "Create";
}

void DrmMap::release()
{
	if (mFd >= 0)
	{
		drmClose(mFd);

		LOG(mLog, DEBUG) << "Delete";
	}
}
