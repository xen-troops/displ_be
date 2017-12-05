/*
 *  Dumb class
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

#ifndef SRC_DRM_DUMB_HPP_
#define SRC_DRM_DUMB_HPP_

#include <xen/be/Log.hpp>
#include <xen/be/XenGnttab.hpp>

#include "DisplayItf.hpp"

namespace Drm {

/***************************************************************************//**
 * Provides DRM dumb functionality.
 * @ingroup drm
 ******************************************************************************/
class DumbBase : public DisplayItf::DisplayBuffer
{
public:

	/**
	 * @param drmFd  DRM file descriptor
	 * @param width  dumb width
	 * @param height dumb height
	 */
	DumbBase(int drmFd, uint32_t width, uint32_t height);

	virtual ~DumbBase() {};

	/**
	 * Returns dumb size
	 */
	size_t getSize() const override { return mSize; }

	/**
	 * Returns pointer to the dumb buffer
	 */
	void* getBuffer() const override { return nullptr; }

	/**
	 * Gets stride
	 */
	uint32_t getStride() const override { return mStride; }

	/**
	 * Gets name
	 */
	uint32_t readName() override;

	/**
	 * Indicates if copy operation shall be applied
	 */
	bool needsCopy() override { return false; };

	/**
	 * Copies data from associated grant table buffer
	 */
	void copy() override;

protected:

	int mDrmFd;
	uint32_t mBufDrmHandle;
	uint32_t mStride;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mName;
	size_t mSize;
	XenBackend::Log mLog;

	void createDumb(uint32_t bpp);
};

/***************************************************************************//**
 * Provides DRM dumb functionality.
 * @ingroup drm
 ******************************************************************************/
class DumbDrm : public DumbBase
{
public:

	/**
	 * @param fd     DRM file descriptor
	 * @param width  dumb width
	 * @param height dumb height
	 * @param bpp    bits per pixel
	 * @param domId  domain id
	 * @param refs   grant table refs
	 */
	DumbDrm(int fd, uint32_t width, uint32_t height,
			uint32_t bpp, domid_t domId = 0,
			const DisplayItf::GrantRefs& refs = DisplayItf::GrantRefs());

	~DumbDrm();

	/**
	 * Returns pointer to the dumb buffer
	 */
	void* getBuffer() const override { return mBuffer; }

	/**
	 * Gets handle
	 */
	uintptr_t getHandle() const override { return mBufDrmHandle; }

	/**
	 * Gets fd
	 */
	virtual int getFd() const { return mDrmFd; }

	/**
	 * Indicates if copy operation shall be applied
	 */
	bool needsCopy() override { return static_cast<bool>(mGnttabBuffer); };

	/**
	 * Copies data from associated grant table buffer
	 */
	void copy() override;

private:

	friend class FrameBuffer;

	void* mBuffer;

	std::unique_ptr<XenBackend::XenGnttabBuffer> mGnttabBuffer;

	void mapDumb();

	void init(uint32_t bpp, domid_t domId, const DisplayItf::GrantRefs& refs);
	void release();
};

#ifdef WITH_ZCOPY

/***************************************************************************//**
 * Provides DRM ZCopy front dumb functionality.
 * @ingroup drm
 ******************************************************************************/
class DumbZCopyFront : public DumbBase
{
public:

	/**
	 * @param drmFd    DRM file descriptor
	 * @param zCopyFd  ZCopy file descriptor
	 * @param width    dumb width
	 * @param height   dumb height
	 * @param bpp      bits per pixel
	 */
	DumbZCopyFront(int drmFd, int zCopyFd,
				   uint32_t width, uint32_t height, uint32_t bpp,
				   domid_t domId, const DisplayItf::GrantRefs& refs);

	~DumbZCopyFront();

	/**
	 * Get handle
	 */
	virtual uintptr_t getHandle() const override { return mBufZCopyHandle; }

	/**
	 * Gets fd
	 */
	int getFd() const override { return mBufZCopyFd; };

private:

	int mZCopyFd;
	uint32_t mBufZCopyHandle;
	uint32_t mBufZCopyFd;

	void createDumb(uint32_t bpp, domid_t domId,
					const DisplayItf::GrantRefs& refs);
	void getBufFd();

	void init(uint32_t bpp, domid_t domId, const DisplayItf::GrantRefs& refs);
	void release();
};

/***************************************************************************//**
 * Provides DRM ZCopy front DRM dumb functionality.
 * @ingroup drm
 ******************************************************************************/
class DumbZCopyFrontDrm : public DumbZCopyFront
{
public:

	/**
	 * @param drmFd    DRM file descriptor
	 * @param zCopyFd  ZCopy file descriptor
	 * @param width    dumb width
	 * @param height   dumb height
	 * @param bpp      bits per pixel
	 */
	DumbZCopyFrontDrm(int drmFd, int zCopyFd,
					  uint32_t width, uint32_t height, uint32_t bpp,
					  domid_t domId, const DisplayItf::GrantRefs& refs);

	~DumbZCopyFrontDrm();

	/**
	 * Get handle
	 */
	virtual uintptr_t getHandle() const override { return mBufDrmHandle; }

	/**
	 * Gets fd
	 */
	int getFd() const override { return mDrmFd; };

protected:

	uint32_t mBufZCopyHandle;
	uint32_t mBufZCopyFd;

private:

	int mZCopyFd;

	void createDumb(uint32_t bpp, domid_t domId,
					const DisplayItf::GrantRefs& refs);
	void createHandle();

	void init(uint32_t bpp, domid_t domId, const DisplayItf::GrantRefs& refs);
	void release();
};

/***************************************************************************//**
 * Provides DRM ZCopy back dumb functionality.
 * @ingroup drm
 ******************************************************************************/
class DumbZCopyBack : public DumbBase
{
public:

	/**
	 * @param drmFd    DRM file descriptor
	 * @param zCopyFd  ZCopy file descriptor
	 * @param width    dumb width
	 * @param height   dumb height
	 * @param bpp      bits per pixel
	 */
	DumbZCopyBack(int drmFd, int zCopyFd,
				  uint32_t width, uint32_t height, uint32_t bpp,
				  domid_t domId, DisplayItf::GrantRefs& refs);

	~DumbZCopyBack();

	/**
	 * Get handle
	 */
	virtual uintptr_t getHandle() const override { return mBufDrmHandle; }

	/**
	 * Gets fd
	 */
	int getFd() const override { return mBufDrmFd; };

protected:

	uint32_t mBufZCopyHandle;
	uint32_t mBufDrmFd;

private:

	int mZCopyFd;

	void createHandle();

	void getGrantRefs(domid_t domId, DisplayItf::GrantRefs& refs);

	void init(uint32_t bpp, domid_t domId, DisplayItf::GrantRefs& refs);
	void release();
};

#endif

}

#endif /* SRC_DRM_DUMB_HPP_ */
