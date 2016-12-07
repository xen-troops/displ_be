/*
 * SharedMemory.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHAREDMEMORY_HPP_
#define SRC_WAYLAND_SHAREDMEMORY_HPP_

#include <memory>

#include <xen/be/Log.hpp>

#include "Registry.hpp"
#include "SharedBuffer.hpp"
#include "SharedFile.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland shared memory class.
 * @ingroup wayland
 ******************************************************************************/
class SharedMemory : public Registry
{
public:

	~SharedMemory();

	/**
	 * Creates shared file
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return
	 */
	std::shared_ptr<SharedFile> createSharedFile(uint32_t width,
												 uint32_t height,
												 uint32_t bpp);

	/**
	 * Creates shared buffer
	 * @param sharedFile  shared file
	 * @param width       width
	 * @param height      height
	 * @param pixelFormat pixel format
	 * @return
	 */
	std::shared_ptr<SharedBuffer> createSharedBuffer(
			std::shared_ptr<SharedFile> sharedFile,
			uint32_t width, uint32_t height, uint32_t pixelFormat);

private:

	friend class Display;

	SharedMemory(wl_registry* registry, uint32_t id, uint32_t version);

	wl_shm* mWlSharedMemory;
	XenBackend::Log mLog;

	void init();
	void release();
};

}

#endif /* SRC_WAYLAND_SHAREDMEMORY_HPP_ */
