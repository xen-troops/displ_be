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

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

#include <xen/be/Log.hpp>

#include "Compositor.hpp"
#include "Connector.hpp"
#include "SharedMemory.hpp"
#include "Shell.hpp"

#include "DisplayItf.hpp"

namespace Wayland {

/***************************************************************************//**
 * @defgroup wayland Wayland
 * Wayland related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Wayland display class.
 * @ingroup wayland
 ******************************************************************************/
class Display : public DisplayItf
{
public:

	Display();
	~Display();

	/**
	 * Creates virtual connector
	 * @param id     connector id
	 * @param x      horizontal offset
	 * @param y      vertical offset
	 * @param width  width
	 * @param height height
	 */
	void createConnector(uint32_t id, uint32_t x, uint32_t y,
						 uint32_t width, uint32_t height);

	/**
	 * Starts events handling
	 */
	void start() override;

	/**
	 * Stops events handling
	 */
	void stop() override;

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

	wl_display* mWlDisplay;
	wl_registry* mWlRegistry;
	wl_registry_listener mWlRegistryListener;
	std::atomic_bool mTerminate;
	XenBackend::Log mLog;

	std::shared_ptr<Compositor> mCompositor;
	std::shared_ptr<Shell> mShell;
	std::shared_ptr<SharedMemory> mSharedMemory;

	std::shared_ptr<ShellSurface> mMainShellSurface;
	std::shared_ptr<SharedBuffer> mMainSharedBuffer;

	std::unordered_map<uint32_t, std::shared_ptr<Connector>> mConnectors;

	std::thread mThread;

	static void sRegistryHandler(void *data, wl_registry *registry,
								 uint32_t id, const char *interface,
								 uint32_t version);
	static void sRegistryRemover(void *data, struct wl_registry *registry,
								 uint32_t id);

	void registryHandler(wl_registry *registry, uint32_t id,
						 const std::string& interface, uint32_t version);
	void registryRemover(wl_registry *registry, uint32_t id);

	void mainShellSurfaceConfigCbk(uint32_t edges, int32_t width,
								   int32_t height);

	void init();
	void release();

	bool pollDisplayFd();
	void dispatchThread();
};

}

#endif /* SRC_WAYLAND_DISPLAY_HPP_ */
