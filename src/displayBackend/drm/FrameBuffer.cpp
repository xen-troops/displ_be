/*
 *  FrameBuffer class
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

#include "FrameBuffer.hpp"

#include <xf86drmMode.h>

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using std::string;

using DisplayItf::DisplayBufferPtr;

namespace Drm {

/*******************************************************************************
 * FrameBuffer
 ******************************************************************************/

FrameBuffer::FrameBuffer(int drmFd, DisplayBufferPtr displayBuffer,
						 uint32_t width, uint32_t height,
						 uint32_t pixelFormat) :
	mDrmFd(drmFd),
	mDisplayBuffer(displayBuffer),
	mWidth(width),
	mHeight(height),
	mId(0),
	mLog("FrameBuffer")
{
	try
	{
		init(pixelFormat);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

FrameBuffer::~FrameBuffer()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void FrameBuffer::init(uint32_t pixelFormat)
{
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};

	handles[0] = mDisplayBuffer->getHandle();
	pitches[0] = mDisplayBuffer->getStride();

	auto ret = drmModeAddFB2(mDrmFd, mWidth, mHeight, pixelFormat,
							 handles, pitches, offsets, &mId, 0);

	if (ret)
	{
		throw Exception ("Cannot create frame buffer", errno);
	}

	DLOG("FrameBuffer", DEBUG) << "Create frame buffer, handle: " << handles[0]
							  << ", id: " << mId;
}

void FrameBuffer::release()
{
	if (mId)
	{
		DLOG("FrameBuffer", DEBUG) << "Delete frame buffer, id: " << mId;

		drmModeRmFB(mDrmFd, mId);
	}
}

}
