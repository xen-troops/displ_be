/*
 * SharedFile.hpp
 *
 *  Created on: Nov 25, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHAREDFILE_HPP_
#define SRC_WAYLAND_SHAREDFILE_HPP_

#include <xen/be/Log.hpp>

namespace Wayland {

class SharedFile
{
public:

	~SharedFile();

	int getFd() const { return mFd; }

	uint32_t getWidth() const { return mWidth; }
	uint32_t getHeight() const { return mHeight; }
	uint32_t getStride() const { return mStride; }

	void* getBuffer() const { return mBuffer; }
	size_t getSize() const { return mSize; }

private:

	friend class SharedMemory;

	SharedFile(uint32_t width, uint32_t height, uint32_t bpp);

	constexpr static const char *cFileNameTemplate = "/weston-shared-XXXXXX";
	constexpr static const char *cXdgRuntimeVar = "XDG_RUNTIME_DIR";

	int mFd;
	void* mBuffer;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mStride;
	size_t mSize;

	XenBackend::Log mLog;

	void init();
	void release();
	void createTmpFile();
};

}

#endif /* SRC_WAYLAND_SHAREDFILE_HPP_ */
