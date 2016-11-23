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

#include <atomic>
#include <functional>

namespace Drm {

typedef std::function<void()> FlipCallback;

class Device;
class Dumb;

/***************************************************************************//**
 * Provides DRM frame buffer functionality.
 * @ingroup drm
 ******************************************************************************/
class FrameBuffer
{
public:

	/**
	 * @param drm         DRM device
	 * @param dumb        reference to the dumb object
	 * @param width       frame buffer width
	 * @param height      frame buffer height
	 * @param pixelFormat frame buffer pixel format
	 * @param pitch       frame buffer pitch
	 */
	FrameBuffer(Device& drm, Dumb& dumb, uint32_t width, uint32_t height,
				uint32_t pixelFormat, uint32_t pitch);

	~FrameBuffer();

	/**
	 * Returns frame buffer id
	 * @return frame buffer id
	 */
	uint32_t getId() const { return mId; }

	/**
	 * Returns reference to the dumb object associated with this frame buffer
	 * @return reference to the dumb object
	 */
	Dumb& getDumb() const { return mDumb; }

	/**
	 * Returns dumb handle associated with this frame buffer
	 * @return dumb handle
	 */
	uint32_t getHandle() const;

	/**
	 * Performs page flip
	 * @param crtcId CRTC id
	 * @param cbk    callback which will be called when page flip is done
	 */
	void pageFlip(uint32_t crtcId, FlipCallback cbk);

private:

	Device& mDrm;
	Dumb& mDumb;
	uint32_t mId;
	std::atomic_bool mFlipPending;
	FlipCallback mFlipCallback;

	friend class Device;

	void flipFinished();
};

}

#endif /* SRC_DRM_FRAMEBUFFER_HPP_ */
