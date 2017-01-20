/*
 * Dumb.hpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#ifndef SRC_DRM_DUMBZEROCOPY_HPP_
#define SRC_DRM_DUMBZEROCOPY_HPP_

#include <vector>

#include <xen/be/XenGnttab.hpp>

#include "DisplayItf.hpp"

namespace Drm {

class DumbZeroCopy : public DisplayItf::DisplayBuffer
{
public:

	/**
	 * @param fd     DRM file descriptor
	 * @param width  dumb width
	 * @param height dumb height
	 * @param bpp    bits per pixel
	 */
	DumbZeroCopy(int drmFd, int mapFd,
				 uint32_t width, uint32_t height, uint32_t bpp,
				 domid_t domId, const DisplayItf::GrantRefs& refs);

	~DumbZeroCopy();

	/**
	 * Returns dumb size
	 */
	size_t getSize() const override { return mSize; }

	/**
	 * Returns pointer to the dumb buffer
	 */
	void* getBuffer() const override { return mBuffer; }

	/**
	 * Get stride
	 */
	virtual uint32_t getStride() const override { return mStride; }

	/**
	 * Get handle
	 */
	virtual uintptr_t getHandle() const override { return mHandle; }

	/**
	 * Gets name
	 */
	uint32_t readName() override;

	/**
	 * Copies data from associated grant table buffer
	 */
	void copy() override;

private:

	friend class FrameBuffer;

	int mDrmFd;
	int mMappedFd;
	uint32_t mHandle;
	uint32_t mMappedHandle;
	uint32_t mStride;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mName;
	size_t mSize;
	void* mBuffer;
	XenBackend::Log mLog;

	void createDumb(uint32_t bpp, domid_t domId,
					const DisplayItf::GrantRefs& refs);
	void createHandle();
	void mapDumb();

	void init(uint32_t bpp, domid_t domId, const DisplayItf::GrantRefs& refs);
	void release();
};

}

#endif /* SRC_DRM_DUMBZEROCOPY_HPP_ */
