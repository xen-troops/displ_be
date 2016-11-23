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

class FrameBuffer
{
public:

	FrameBuffer(Device& drm, Dumb& dumb, uint32_t width, uint32_t height,
				uint32_t pixelFormat, uint32_t pitch);
	~FrameBuffer();

	uint32_t getId() const { return mId; }
	Dumb& getDumb() const { return mDumb; }
	uint32_t getHandle() const;

	void pageFlip(uint32_t crtc, FlipCallback cbk);

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
