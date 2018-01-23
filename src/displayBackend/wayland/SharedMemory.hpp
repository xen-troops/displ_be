/*
 * SharedMemory.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHAREDMEMORY_HPP_
#define SRC_WAYLAND_SHAREDMEMORY_HPP_

#include <list>
#include <memory>

#include <xen/be/Log.hpp>

#include "FrameBuffer.hpp"
#include "Registry.hpp"
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
	SharedFilePtr createSharedFile(
			uint32_t width, uint32_t height, uint32_t bpp, domid_t domId = 0,
			const DisplayItf::GrantRefs& refs = DisplayItf::GrantRefs());

	/**
	 * Creates shared buffer
	 * @param sharedFile  shared file
	 * @param width       width
	 * @param height      height
	 * @param pixelFormat pixel format
	 * @return
	 */
	SharedBufferPtr createSharedBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width, uint32_t height, uint32_t pixelFormat);

private:

	friend class Display;

	SharedMemory(wl_registry* registry, uint32_t id, uint32_t version);

	wl_shm* mWlSharedMemory;
	XenBackend::Log mLog;

	wl_shm_listener mWlListener;

	std::list<uint32_t> mSupportedFormats;

	static void sFormatHandler(void *data, wl_shm *wlShm, uint32_t format);

	void formatHandler(uint32_t format);

	void init();
	void release();

	uint32_t convertPixelFormat(uint32_t format);
	bool isPixelFormatSupported(uint32_t format);
};

typedef std::shared_ptr<SharedMemory> SharedMemoryPtr;

}

#endif /* SRC_WAYLAND_SHAREDMEMORY_HPP_ */
