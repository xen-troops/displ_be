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

#include <memory>
#include <thread>
#include <unordered_map>

#include "DisplayItf.hpp"
#include "Connector.hpp"
#include "FrameBuffer.hpp"

namespace Drm {

/***************************************************************************//**
 * @defgroup drm DRM
 * DRM related classes.
 ******************************************************************************/

/***************************************************************************//**
 * DRM Device class.
 * @ingroup drm
 ******************************************************************************/
class Device : public DisplayItf
{
public:

	/**
	 * @param name device name
	 */
	explicit Device(const std::string& name);

	~Device();

	/**
	 * Returns opened DRM file descriptor
	 */
	int getFd() const { return mFd; }

	/**
	 * Returns DRM magic
	 */
	drm_magic_t getMagic();

	/**
	 * Returns number of supported CRTC's
	 */
	int getCtrcsCount() const { return (*mRes)->count_crtcs; }

	/**
	 * Returns CRTC id by index
	 * @param index index in CRTC array
	 */
	uint32_t getCtrcIdByIndex(int index) const { return (*mRes)->crtcs[index]; }

	/**
	 * Returns connector by index
	 * @param index index in arrays
	 */
	std::shared_ptr<Connector> getConnectorByIndex(uint32_t index);

	/**
	 * Returns number of connectors
	 */
	size_t getConnectorsCount();

	/**
	 * Creates virtual connector
	 * @param id       connector id
	 * @param drmId    DRM connector id
	 * @return created connector
	 */
	std::shared_ptr<ConnectorItf> createConnector(uint32_t id, uint32_t drmId);

	/**
	 * Starts events handling
	 */
	void start() override;

	/**
	 * Stops events handling
	 */
	void stop() override;

	/**
	 * Returns if display supports zero copy buffers
	 */
	bool isZeroCopySupported() const override { return (mZeroCopyFd >= 0); }

	/**
	 * Returns connector by id
	 * @param id connector id
	 */
	std::shared_ptr<ConnectorItf> getConnectorById(uint32_t id) override;

	/**
	 * Creates display buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	std::shared_ptr<DisplayBufferItf> createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp) override;

	/**
	 * Creates display buffer with associated grand table buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	std::shared_ptr<DisplayBufferItf> createDisplayBuffer(
			domid_t domId, const std::vector<grant_ref_t>& refs,
			uint32_t width, uint32_t height, uint32_t bpp) override;

	/**
	 * Creates frame buffer
	 * @param displayBuffer pointer to the display buffer
	 * @param width         width
	 * @param height        height
	 * @param pixelFormat   pixel format
	 * @return shared pointer to the frame buffer
	 */
	std::shared_ptr<FrameBufferItf> createFrameBuffer(
			std::shared_ptr<DisplayBufferItf> displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat) override;

private:

	const int cPoolEventTimeoutMs = 100;

	std::string mName;
	int mFd;
	int mZeroCopyFd;
	std::atomic_bool mTerminate;
	XenBackend::Log mLog;

	std::unique_ptr<ModeResource> mRes;

	std::unordered_map<uint32_t, std::shared_ptr<Connector>> mConnectors;

	std::mutex mMutex;
	std::thread mThread;

	void init();
	void release();
	void eventThread();

	static void handleFlipEvent(int fd, unsigned int sequence,
								unsigned int tv_sec, unsigned int tv_usec,
								void *user_data);

	friend class FrameBuffer;

	bool isStopped() { return mTerminate; }
};

}

#endif /* SRC_DRM_DEVICE_HPP_ */
