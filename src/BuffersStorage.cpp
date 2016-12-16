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

using std::hex;
using std::lock_guard;
using std::move;
using std::mutex;
using std::setfill;
using std::setw;
using std::shared_ptr;
using std::vector;

using XenBackend::XenGnttabBuffer;

/*******************************************************************************
 * BuffersStorage
 ******************************************************************************/

BuffersStorage::BuffersStorage(int domId, std::shared_ptr<DisplayItf> display) :
	mDomId(domId),
	mDisplay(display),
	mLog("BuffersStorage")
{

}

/*******************************************************************************
 * Public
 ******************************************************************************/

void BuffersStorage::createDisplayBuffer(uint64_t dbCookie,
										 grant_ref_t startDirectory,
										 uint32_t size,
										 uint32_t width, uint32_t height,
										 uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Create display buffer, w: "
					  << width << ", h: " << height << ", bpp: " << bpp
					  << ", start dir: " << startDirectory
					  << ", size: " << size << ", DB cookie: "
					  << hex << setfill('0') << setw(16)
					  << dbCookie;

	LocalDisplayBuffer displayBuffer {
		mDisplay->createDisplayBuffer(width, height, bpp) };

	vector<grant_ref_t> refs;

	getBufferRefs(startDirectory, size, refs);

	displayBuffer.buffer.reset(new XenGnttabBuffer(mDomId, refs.data(),
												   refs.size()));

	mDisplayBuffers.emplace(dbCookie, move(displayBuffer));
}


void BuffersStorage::createFrameBuffer(uint64_t dbCookie, uint64_t fbCookie,
									   uint32_t width, uint32_t height,
									   uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Create frame buffer, w: " << width
					  << ", h: " << height
					  << ", pixel fmt: " << hex << setfill('0') << setw(8)
					  << pixelFormat
					  << ", DB cookie: " << setw(16) << dbCookie
					  << ", FB cookie: " << setw(16) << fbCookie;

	auto frameBuffer =
			mDisplay->createFrameBuffer(getDisplayBufferUnlocked(dbCookie),
									    width, height, pixelFormat);

	mFrameBuffers.emplace(fbCookie, frameBuffer);
}

shared_ptr<DisplayBufferItf> BuffersStorage::getDisplayBuffer(uint64_t dbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Get display buffer, DB cookie: "
					  << hex << setfill('0') << setw(16) << dbCookie;

	return getDisplayBufferUnlocked(dbCookie);
}

shared_ptr<FrameBufferItf> BuffersStorage::getFrameBuffer(uint64_t fbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Get frame buffer, FB cookie: "
					  << hex << setfill('0') << setw(16) << fbCookie;

	return getFrameBufferUnlocked(fbCookie);
}

void BuffersStorage::destroyDisplayBuffer(uint64_t dbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Destroy display buffer, DB cookie: "
					  << hex << setfill('0') << setw(16) << dbCookie;

	mDisplayBuffers.erase(dbCookie);
}

void BuffersStorage::destroyFrameBuffer(uint64_t fbCookie)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Destroy frame buffer, FB cookie: "
					  << hex << setfill('0') << setw(16) << fbCookie;

	mFrameBuffers.erase(fbCookie);
}

void BuffersStorage::copyBuffer(uint64_t fbCookie)
{
	lock_guard<mutex> lock(mMutex);

	auto displayBuffer = getFrameBufferUnlocked(fbCookie)->getDisplayBuffer();

	auto iter = mDisplayBuffers.begin();

	for (; iter != mDisplayBuffers.end(); iter++)
	{
		if (displayBuffer == iter->second.displayBuffer)
		{
			DLOG(mLog, DEBUG) << "Copy buffer, size: "
							  << displayBuffer->getSize()
							  << ", FB cookie: "
							  << hex << setfill('0') << setw(16) << fbCookie;

			memcpy(displayBuffer->getBuffer(), iter->second.buffer->get(),
					displayBuffer->getSize());

			return;
		}
	}

	throw DisplayItfException("Handle not found");
}

/*******************************************************************************
 * Private
 ******************************************************************************/

shared_ptr<DisplayBufferItf>
	BuffersStorage::getDisplayBufferUnlocked(uint64_t dbCookie)
{
	auto iter = mDisplayBuffers.find(dbCookie);

	if (iter == mDisplayBuffers.end())
	{
		throw DisplayItfException("Dumb cookie not found");
	}

	return iter->second.displayBuffer;
}

shared_ptr<FrameBufferItf>
	BuffersStorage::getFrameBufferUnlocked(uint64_t fbCookie)
{
	auto iter = mFrameBuffers.find(fbCookie);

	if (iter == mFrameBuffers.end())
	{
		throw DisplayItfException("Frame buffer cookie not found");
	}

	return iter->second;
}

void BuffersStorage::getBufferRefs(grant_ref_t startDirectory, uint32_t size,
								   vector<grant_ref_t>& refs)
{
	refs.clear();

	size_t requestedNumGrefs = (size + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE;

	DLOG(mLog, DEBUG) << "Get buffer refs, directory: " << startDirectory
					  << ", size: " << size
					  << ", in grefs: " << requestedNumGrefs;


	while(startDirectory != 0)
	{

		XenGnttabBuffer pageBuffer(mDomId, startDirectory);

		xendispl_page_directory* pageDirectory =
				static_cast<xendispl_page_directory*>(pageBuffer.get());

		size_t numGrefs = min(requestedNumGrefs,(XC_PAGE_SIZE -
							  offsetof(xendispl_page_directory, gref)) /
							  sizeof(uint32_t));

		refs.insert(refs.end(), pageDirectory->gref,
					pageDirectory->gref + numGrefs);

		requestedNumGrefs -= numGrefs;

		startDirectory = pageDirectory->gref_dir_next_page;
	}

	DLOG(mLog, DEBUG) << "Get buffer refs, num refs: " << refs.size();
}

