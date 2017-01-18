/*
 * DrmBuffer.hpp
 *
 *  Created on: Jan 17, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_DRMBUFFER_HPP_
#define SRC_WAYLAND_DRMBUFFER_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"
#include "wayland-drm/wayland-drm-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * DRM buffer class.
 * @ingroup wayland
 ******************************************************************************/
class DrmBuffer : public FrameBufferItf
{
public:

	~DrmBuffer();

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
	DisplayBufferPtr getDisplayBuffer() override
	{
		return mDisplayBuffer;
	}

private:

	friend class WaylandDrm;

	DrmBuffer(wl_drm* wlDrm, DisplayBufferPtr displayBuffer,
			  uint32_t width, uint32_t height, uint32_t pixelFormat);

	DisplayBufferPtr mDisplayBuffer;
	wl_buffer* mWlBuffer;
	uint32_t mWidth;
	uint32_t mHeight;
	XenBackend::Log mLog;

	void init(wl_drm* wlDrm, uint32_t pixelFormat);
	void release();
};

}

#endif /* SRC_WAYLAND_DRMBUFFER_HPP_ */
