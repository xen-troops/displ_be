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

#ifndef SRC_DRM_FRAMEBUFFER_HPP_
#define SRC_DRM_FRAMEBUFFER_HPP_

#include <memory>
#include <functional>

#include "DisplayItf.hpp"

namespace Drm {

class Device;
class Dumb;

/***************************************************************************//**
 * Provides DRM frame buffer functionality.
 * @ingroup drm
 ******************************************************************************/
class FrameBuffer : public FrameBufferItf
{
public:

	/**
	 * @param dumb        dumb
	 * @param width       frame buffer width
	 * @param height      frame buffer height
	 * @param pixelFormat frame buffer pixel format
	 */
	FrameBuffer(std::shared_ptr<Dumb> dumb, uint32_t width, uint32_t height,
				uint32_t pixelFormat);

	~FrameBuffer();

	/**
	 * Returns frame buffer id
	 */
	uint32_t getId() const { return mId; }

	/**
	 * Returns pointer to the display buffer
	 */
	std::shared_ptr<DisplayBufferItf> getDisplayBuffer() override
	{
		return std::dynamic_pointer_cast<DisplayBufferItf>(mDumb);
	}

private:

	std::shared_ptr<Dumb> mDumb;
	uint32_t mId;
};

}

#endif /* SRC_DRM_FRAMEBUFFER_HPP_ */
