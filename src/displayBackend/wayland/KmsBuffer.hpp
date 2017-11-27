/*
 *  Kms buffer class
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

#ifndef SRC_WAYLAND_KMSBUFFER_HPP_
#define SRC_WAYLAND_KMSBUFFER_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"
#include "wayland-kms-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * KMS buffer class.
 * @ingroup wayland
 ******************************************************************************/
class KmsBuffer : public DisplayItf::FrameBuffer
{
public:

	~KmsBuffer();

	/**
	 * Gets handle
	 */
	uintptr_t getHandle() const override
	{
		return reinterpret_cast<uintptr_t>(mWlBuffer);
	}

	/**
	 * Gets width
	 */
	uint32_t getWidth() const override { return mWidth; }

	/**
	 * Gets width
	 */
	uint32_t getHeight() const override { return mHeight; };

	/**
	 * Returns pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr getDisplayBuffer() override
	{
		return mDisplayBuffer;
	}

private:

	friend class WaylandKms;

	KmsBuffer(wl_kms* wlKms, DisplayItf::DisplayBufferPtr displayBuffer,
			  uint32_t width, uint32_t height, uint32_t pixelFormat);

	DisplayItf::DisplayBufferPtr mDisplayBuffer;
	wl_buffer* mWlBuffer;
	uint32_t mWidth;
	uint32_t mHeight;
	XenBackend::Log mLog;

	void init(wl_kms* wlKms, uint32_t pixelFormat);
	void release();
};

}

#endif /* SRC_WAYLAND_KMSBUFFER_HPP_ */
