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

class Device
{
public:
	Device(const std::string& name);
	~Device();

	int getFd() const { return mFd; }

	int getCtrcsCount() const { return (*mRes)->count_crtcs; }
	uint32_t getCtrcIdByIndex(int index) const { return (*mRes)->crtcs[index]; }

	Connector& getConnectorById(uint32_t id);
	Connector& getConnectorByIndex(uint32_t index);
	size_t getConnectorsCount();

	void start();
	void stop();

	Dumb& createDumb(uint32_t width, uint32_t height,
									 uint32_t bpp);
	void deleteDumb(uint32_t handle);

	FrameBuffer& createFrameBuffer(Dumb& dumb, uint32_t width,
								   uint32_t height, uint32_t pixelFormat);
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
