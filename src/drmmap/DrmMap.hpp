/*
 * DrmMap.hpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#ifndef SRC_DRMMAP_DRMMAP_HPP_
#define SRC_DRMMAP_DRMMAP_HPP_

#include <memory>
#include <vector>

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"

class DrmMap
{
public:
	DrmMap(int drmFd);
	~DrmMap();

	std::shared_ptr<DisplayBufferItf> createDisplayBuffer(
			int domId, const std::vector<uint32_t>& refs, uint32_t width,
			uint32_t height, uint32_t bpp);

private:
	int mFd;
	int mDrmFd;
	XenBackend::Log mLog;

	void init();
	void release();
};

#endif /* SRC_DRMMAP_DRMMAP_HPP_ */
