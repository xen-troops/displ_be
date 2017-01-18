/*
 * SharedBuffer.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHAREDBUFFER_HPP_
#define SRC_WAYLAND_SHAREDBUFFER_HPP_

#include <memory>

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "../DisplayItf.hpp"

namespace Wayland {

/***************************************************************************//**
 * Shared buffer class.
 * @ingroup wayland
 ******************************************************************************/
class SharedBuffer : public DisplayItf::FrameBuffer
{
public:

	~SharedBuffer();

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

	friend class SharedMemory;

	SharedBuffer(wl_shm* wlSharedMemory,
				 DisplayItf::DisplayBufferPtr displayBuffer,
				 uint32_t width, uint32_t height,
				 uint32_t pixelFormat);

	DisplayItf::DisplayBufferPtr mDisplayBuffer;
	wl_buffer* mWlBuffer;
	wl_shm_pool* mWlPool;
	uint32_t mWidth;
	uint32_t mHeight;
	XenBackend::Log mLog;

	uint32_t convertPixelFormat(uint32_t format);
	void init(wl_shm* wlSharedMemory, uint32_t pixelFormat);
	void release();
};

}

#endif /* SRC_WAYLAND_SHAREDBUFFER_HPP_ */
