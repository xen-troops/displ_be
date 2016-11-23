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

#include <map>
#include <memory>
#include <thread>

#include "Connector.hpp"
#include "Dumb.hpp"
#include "FrameBuffer.hpp"

namespace Drm {

/***************************************************************************//**
 * @defgroup drm
 * DRM related classes.
 ******************************************************************************/

/***************************************************************************//**
 * DRM Device class.
 * @ingroup drm
 ******************************************************************************/
class Device
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
	 * Returns number of supported CRTC's
	 * @return number of supported CRTC's
	 */
	int getCtrcsCount() const { return (*mRes)->count_crtcs; }

	/**
	 * Returns CRTC id by index
	 * @param index index in CRTC array
	 * @return CRTC id
	 */
	uint32_t getCtrcIdByIndex(int index) const { return (*mRes)->crtcs[index]; }

	/**
	 * Returns connector by id
	 * @param id connector id
	 * @return connector
	 */
	Connector& getConnectorById(uint32_t id);

	/**
	 * Returns connector by index
	 * @param index index in array
	 * @return connector
	 */
	Connector& getConnectorByIndex(uint32_t index);

	/**
	 * Returns number of connectors
	 * @return number of connectors
	 */
	size_t getConnectorsCount();

	/**
	 * Starts DRM flip page events handling
	 */
	void start();

	/**
	 * Stops DRM flip page events handling
	 */
	void stop();

	/**
	 * Creates DRM dumb
	 * @param width  width of the dumb buffer
	 * @param height height of the dumb buffer
	 * @param bpp    bits per pixel of the dumb buffer
	 * @return reference to the dumb object
	 */
	Dumb& createDumb(uint32_t width, uint32_t height,
									 uint32_t bpp);

	/**
	 * Deletes the dumb by handle
	 * @param handle dumb handle
	 */
	void deleteDumb(uint32_t handle);

	/**
	 * Creates DRM frame buffer
	 * @param dumb        reference to the dumb object
	 * @param width       width of the frame buffer
	 * @param height      height of the frame buffer
	 * @param pixelFormat pixel format of the frame buffer
	 * @return reference to the frame buffer object
	 */
	FrameBuffer& createFrameBuffer(Dumb& dumb, uint32_t width,
								   uint32_t height, uint32_t pixelFormat);

	/**
	 * Deletes the frame buffer by id
	 * @param id id of the frame buffer
	 */
	void deleteFrameBuffer(uint32_t id);

private:

	const int cPoolEventTimeoutMs = 100;

	std::string mName;
	int mFd;
	std::atomic_bool mTerminate;
	std::atomic_int mNumFlipPages;
	XenBackend::Log mLog;

	std::unique_ptr<ModeResource> mRes;

	std::map<uint32_t, std::unique_ptr<Connector>> mConnectors;
	std::map<uint32_t, std::unique_ptr<Dumb>> mDumbs;
	std::map<uint32_t, std::unique_ptr<FrameBuffer>> mFrameBuffers;

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
	void pageFlipScheduled() { mNumFlipPages++; }
	void pageFlipDone() { mNumFlipPages--; }
};

}

#endif /* SRC_DRM_DEVICE_HPP_ */
