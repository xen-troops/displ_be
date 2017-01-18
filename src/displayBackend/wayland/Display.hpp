/*
 *  Display class
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

#ifndef SRC_WAYLAND_DISPLAY_HPP_
#define SRC_WAYLAND_DISPLAY_HPP_

#include "DisplayItf.hpp"

#include <atomic>
#include <thread>
#include <unordered_map>

#include <xen/be/Log.hpp>

#include "Compositor.hpp"
#include "Connector.hpp"
#include "IviApplication.hpp"
#include "Seat.hpp"
#include "SharedMemory.hpp"
#include "Shell.hpp"
#include "WaylandDrm.hpp"

namespace Wayland {

/***************************************************************************//**
 * @defgroup wayland Wayland
 * Wayland related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Wayland display class.
 * @ingroup wayland
 ******************************************************************************/
class Display : public DisplayItf::Display
{
public:

	Display();
	~Display();

	/**
	 * Creates background surface
	 * @param width  width
	 * @param height height
	 */
	void createBackgroundSurface(uint32_t width, uint32_t height);

	/**
	 * Creates virtual connector
	 * @param id         connector id
	 * @param x          horizontal offset
	 * @param y          vertical offset
	 * @param width      width
	 * @param height     height
	 * @return created connector
	 */
	DisplayItf::ConnectorPtr createConnector(uint32_t id, uint32_t x,
											 uint32_t y, uint32_t width,
											 uint32_t height);

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
	bool isZeroCopySupported() const override;

	/**
	 * Returns connector by id
	 * @param id connector id
	 */
	DisplayItf::ConnectorPtr getConnectorById(uint32_t id) override;

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
	 * Creates display buffer with associated grant table buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr createDisplayBuffer(
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
	DisplayItf::FrameBufferPtr createFrameBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat) override;

	SeatPtr getSeat() const { return mSeat; }

private:

	const int cPoolEventTimeoutMs = 100;

	wl_display* mWlDisplay;
	wl_registry* mWlRegistry;
	wl_registry_listener mWlRegistryListener;
	std::atomic_bool mTerminate;
	XenBackend::Log mLog;

	std::unordered_map<uint32_t, Wayland::ConnectorPtr> mConnectors;

	CompositorPtr mCompositor;
	ShellPtr mShell;
	SharedMemoryPtr mSharedMemory;
	IviApplicationPtr mIviApplication;
	SeatPtr mSeat;
	WaylandDrmPtr mWaylandDrm;

	ShellSurfacePtr mBackgroundSurface;

	std::thread mThread;

	ShellSurfacePtr createShellSurface(uint32_t x, uint32_t y);
	IviSurfacePtr createIviSurface(uint32_t x, uint32_t y,
								   uint32_t width, uint32_t height);

	static void sRegistryHandler(void *data, wl_registry *registry,
								 uint32_t id, const char *interface,
								 uint32_t version);
	static void sRegistryRemover(void *data, struct wl_registry *registry,
								 uint32_t id);

	void registryHandler(wl_registry *registry, uint32_t id,
						 const std::string& interface, uint32_t version);
	void registryRemover(wl_registry *registry, uint32_t id);

	void init();
	void release();

	bool pollDisplayFd();
	void dispatchThread();
};

typedef std::shared_ptr<Display> DisplayPtr;

}

#endif /* SRC_WAYLAND_DISPLAY_HPP_ */
