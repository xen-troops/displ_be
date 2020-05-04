/*
 *  Buffers storage
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
 */

#include "BuffersStorage.hpp"

#include <iomanip>
#include <vector>

#include <drm_fourcc.h>

#include <xen/be/Exception.hpp>

using std::hex;
using std::lock_guard;
using std::move;
using std::mutex;
using std::setfill;
using std::setw;

using XenBackend::XenGnttabBuffer;

using DisplayItf::DisplayPtr;
using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;

/*******************************************************************************
 * BuffersStorage
 ******************************************************************************/

BuffersStorage::BuffersStorage(domid_t domId, DisplayPtr display) :
	mDomId(domId),
	mDisplay(display),
	mLog("BuffersStorage")
{

}

BuffersStorage::~BuffersStorage()
{
	mFrameBuffers.clear();

	mDisplay->flush();

	mDisplayBuffers.clear();
	mPendingDisplayBuffers.clear();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void BuffersStorage::createDisplayBuffer(uint64_t dbCookie, bool beAllocRefs,
										 grant_ref_t startDirectory,
										 size_t offset, uint32_t size,
										 uint32_t width, uint32_t height,
										 uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	GrantRefs refs;

	if (!beAllocRefs)
	{
		pgDirGetBufferRefs(mDomId, startDirectory, size, refs);
	}

	if (width == 0)
	{
		if (beAllocRefs)
		{
			throw XenBackend::Exception("Can't create pending display buffer",
										EINVAL);
		}

		DLOG(mLog, DEBUG) << "Create pending display buffer, start dir: "
						  << startDirectory
						  << ", size: " << size << ", offset: " << offset
						  << ", DB cookie: 0x"
						  << hex << setfill('0') << setw(16)
						  << dbCookie;

		mPendingDisplayBuffers.emplace(dbCookie, PendingBuffer{offset, refs});
	}
	else
	{
		DLOG(mLog, DEBUG) << "Create display buffer, w: "
						  << width << ", h: " << height << ", bpp: " << bpp
						  << ", offset: " << offset
						  << ", start dir: " << startDirectory
						  << ", size: " << size << ", DB cookie: 0x"
						  << hex << setfill('0') << setw(16)
						  << dbCookie;

		auto displayBuffer = mDisplay->createDisplayBuffer(width, height, bpp,
														   offset, mDomId, refs,
														   beAllocRefs);

		mDisplayBuffers.emplace(dbCookie, displayBuffer);

		if (beAllocRefs)
		{
			pgDirSetBufferRefs(mDomId, startDirectory, size, refs);
		}
	}
}

void BuffersStorage::createFrameBuffer(uint64_t dbCookie, uint64_t fbCookie,
									   uint32_t width, uint32_t height,
									   uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Create frame buffer, w: " << width
					  << ", h: " << height
					  << ", pixel fmt: 0x" << hex << setfill('0') << setw(8)
					  << pixelFormat
					  << ", DB cookie: " << setw(16) << dbCookie
					  << ", FB cookie: " << setw(16) << fbCookie;

	handlePendingDisplayBuffers(dbCookie, width, height, pixelFormat);

	auto frameBuffer = mDisplay->createFrameBuffer(
			getDisplayBufferUnlocked(dbCookie), width, height, pixelFormat);

	mFrameBuffers.emplace(fbCookie, frameBuffer);
}

DisplayBufferPtr BuffersStorage::getDisplayBuffer(uint64_t dbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Get display buffer, DB cookie: 0x"
					  << hex << setfill('0') << setw(16) << dbCookie;

	return getDisplayBufferUnlocked(dbCookie);
}

FrameBufferPtr BuffersStorage::getFrameBufferAndCopy(uint64_t fbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Get frame buffer and copy, FB cookie: 0x"
					  << hex << setfill('0') << setw(16) << fbCookie;

	auto frameBuffer = getFrameBufferUnlocked(fbCookie);

	if (frameBuffer->getDisplayBuffer()->needsCopy())
	{
		frameBuffer->getDisplayBuffer()->copy();
	}

	return frameBuffer;
}

void BuffersStorage::destroyDisplayBuffer(uint64_t dbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Destroy display buffer, DB cookie: 0x"
					  << hex << setfill('0') << setw(16) << dbCookie;

	mDisplayBuffers.erase(dbCookie);
	mPendingDisplayBuffers.erase(dbCookie);
}

void BuffersStorage::destroyFrameBuffer(uint64_t fbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Destroy frame buffer, FB cookie: 0x"
					  << hex << setfill('0') << setw(16) << fbCookie;

	mFrameBuffers.erase(fbCookie);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

uint32_t BuffersStorage::getBpp(uint32_t format)
{
	switch (format)
	{
	case DRM_FORMAT_C8:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_BGR233:
		return 8;

	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_YUYV:
		return 16;

	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;

	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRX1010102:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
		return 32;

	default:
		throw XenBackend::Exception("Invalid pixel format", EINVAL);
	}
}

void BuffersStorage::handlePendingDisplayBuffers(uint64_t dbCookie,
												 uint32_t width,
												 uint32_t height,
												 uint32_t pixelFormat)
{
	if (mPendingDisplayBuffers.size() == 0)
	{
		return;
	}

	auto iter = mPendingDisplayBuffers.find(dbCookie);

	if (iter != mPendingDisplayBuffers.end())
	{
		auto bpp = getBpp(pixelFormat);

		DLOG(mLog, DEBUG) << "Create display buffer from pending, w: "
						  << width << ", h: " << height << ", bpp: " << bpp
						  << ", offset: " << iter->second.offset
						  << ", DB cookie: 0x"
						  << hex << setfill('0') << setw(16)
						  << dbCookie;

		auto displayBuffer = mDisplay->createDisplayBuffer(width, height, bpp,
														   iter->second.offset,
														   mDomId,
														   iter->second.refs,
														   false);

		mDisplayBuffers.emplace(dbCookie, displayBuffer);

		mPendingDisplayBuffers.erase(iter);
	}
}

DisplayBufferPtr BuffersStorage::getDisplayBufferUnlocked(uint64_t dbCookie)
{
	auto iter = mDisplayBuffers.find(dbCookie);

	if (iter == mDisplayBuffers.end())
	{
		throw XenBackend::Exception("Dumb cookie not found", ENOENT);
	}

	return iter->second;
}

FrameBufferPtr BuffersStorage::getFrameBufferUnlocked(uint64_t fbCookie)
{
	auto iter = mFrameBuffers.find(fbCookie);

	if (iter == mFrameBuffers.end())
	{
		throw XenBackend::Exception("Frame buffer cookie not found", ENOENT);
	}

	return iter->second;
}
