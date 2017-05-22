/*
 *  SharedFile class
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

#ifndef SRC_WAYLAND_SHAREDFILE_HPP_
#define SRC_WAYLAND_SHAREDFILE_HPP_

#include <xen/be/Log.hpp>
#include <xen/be/XenGnttab.hpp>

#include "DisplayItf.hpp"

namespace Wayland {

/***************************************************************************//**
 * Shared file class.
 * @ingroup wayland
 ******************************************************************************/
class SharedFile : public DisplayItf::DisplayBuffer
{
public:

	~SharedFile();

	/**
	 * Returns pointer to the beginning of buffer
	 */
	void* getBuffer() const override { return mBuffer; }

	/**
	 * Returns buffer size
	 */
	size_t getSize() const override { return mSize; }

	/**
	 * Get stride
	 */
	virtual uint32_t getStride() const override { return mStride; }

	/**
	 * Get handle
	 */
	virtual uintptr_t getHandle() const override { return mFd; }

	/**
	 * Reads name
	 */
	uint32_t readName() override { return 0; }

	/**
	 * Copies data from associated grant table buffer
	 */
	void copy() override;

private:

	friend class SharedMemory;
	friend class SharedBuffer;

	SharedFile(
			uint32_t width, uint32_t height, uint32_t bpp,
			domid_t domId, const DisplayItf::GrantRefs& refs);

	constexpr static const char *cFileNameTemplate = "/weston-shared-XXXXXX";
	constexpr static const char *cXdgRuntimeVar = "XDG_RUNTIME_DIR";

	int mFd;
	void* mBuffer;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mStride;
	size_t mSize;

	XenBackend::Log mLog;

	std::unique_ptr<XenBackend::XenGnttabBuffer> mGnttabBuffer;

	void init(domid_t domId, const DisplayItf::GrantRefs& refs);
	void release();
	void createTmpFile();
};

typedef std::shared_ptr<SharedFile> SharedFilePtr;

}

#endif /* SRC_WAYLAND_SHAREDFILE_HPP_ */
