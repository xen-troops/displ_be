/*
 * Dumb.hpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#ifndef SRC_DRMMAP_DUMB_HPP_
#define SRC_DRMMAP_DUMB_HPP_

#include <vector>

#include "DisplayItf.hpp"

class Dumb : public DisplayBufferItf
{
public:

	/**
	 * @param fd     DRM file descriptor
	 * @param width  dumb width
	 * @param height dumb height
	 * @param bpp    bits per pixel
	 */
	Dumb(int mapFd, int drmFd, int domId, const std::vector<uint32_t>& refs,
		 uint32_t width, uint32_t height, uint32_t bpp);

	~Dumb();

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
	virtual int getHandle() const override { return mHandle; }

private:

	friend class FrameBuffer;

	int mFd;
	int mMappedFd;
	uint32_t mHandle;
	uint32_t mMappedHandle;
	uint32_t mStride;
	uint32_t mWidth;
	uint32_t mHeight;
	size_t mSize;
	void* mBuffer;

	void release();
};

#endif /* SRC_DRMMAP_DUMB_HPP_ */
