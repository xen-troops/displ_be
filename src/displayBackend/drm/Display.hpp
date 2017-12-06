/*
 *  Device class
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

#ifndef SRC_DRM_DEVICE_HPP_
#define SRC_DRM_DEVICE_HPP_

#include <atomic>
#include <thread>
#include <unordered_map>

#include <xen/be/Utils.hpp>

#include "Connector.hpp"
#include "DisplayItf.hpp"
#include "FrameBuffer.hpp"

namespace Drm {

/***************************************************************************//**
 * @defgroup drm DRM
 * DRM related classes.
 ******************************************************************************/

/***************************************************************************//**
 * DRM Display class.
 * @ingroup drm
 ******************************************************************************/
class Display : public DisplayItf::Display
{
public:

	/**
	 * @param name device name
	 */
	explicit Display(const std::string& name);

	~Display();

	/**
	 * Returns opened DRM file descriptor
	 */
	int getFd() const { return mFd; }

	/**
	 * Returns DRM magic
	 */
	drm_magic_t getMagic();

	/**
	 * Starts events handling
	 */
	void start() override;

	/**
	 * Stops events handling
	 */
	void stop() override;

	/**
	 * Flushes events
	 */
	void flush() override;

	/**
	 * Returns if display supports zero copy buffers
	 */
	bool isZeroCopySupported() const override { return (mZeroCopyFd >= 0); }

	/**
	 * Creates connector
	 * @param name connector name
	 */
	DisplayItf::ConnectorPtr createConnector(const std::string& name) override;

	/**
	 * Creates display buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp) override;

	/**
	 * Creates display buffer with associated grand table buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp,
			domid_t domId, DisplayItf::GrantRefs& refs,
			bool allocRefs) override;

	/**
	 * Creates frame buffer
	 * @param displayBuffer pointer to the display buffer
	 * @param width         width
	 * @param height        height
	 * @param pixelFormat   pixel format
	 * @return shared pointer to the frame buffer
	 */
	DisplayItf::FrameBufferPtr createFrameBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat) override;

private:

	static std::unordered_map<int, std::string> sConnectorNames;

	std::string mName;
	int mFd;
	int mZeroCopyFd;
	std::atomic_bool mStarted;
	XenBackend::Log mLog;

	std::mutex mMutex;
	std::thread mThread;

	std::unique_ptr<XenBackend::PollFd> mPollFd;

	std::unordered_map<std::string, uint32_t> mConnectorIds;

	void init();
	void release();
	void getConnectorIds();
	void eventThread();

	static void handleFlipEvent(int fd, unsigned int sequence,
								unsigned int tv_sec, unsigned int tv_usec,
								void *user_data);

	friend class FrameBuffer;
};

typedef std::shared_ptr<Display> DisplayPtr;

}

#endif /* SRC_DRM_DEVICE_HPP_ */
