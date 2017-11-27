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

#include <thread>
#include <unordered_map>

#include <xen/be/Log.hpp>
#include <xen/be/Utils.hpp>

#include "Compositor.hpp"
#include "Connector.hpp"
#include "DisplayItf.hpp"
#ifdef WITH_IVI_EXTENSION
#include "IviApplication.hpp"
#endif
#ifdef WITH_INPUT
#include "Seat.hpp"
#endif
#include "SharedMemory.hpp"
#include "Shell.hpp"
#ifdef WITH_DRM
#include "WaylandDrm.hpp"
#include "WaylandKms.hpp"
#endif

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
	 * Creates display buffer with associated grant table buffer
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

#ifdef WITH_INPUT

	template <typename T>
	void setInputCallbacks(const std::string& connector, const T& callbacks);
	template <typename T>
	void clearInputCallbacks(const std::string& connector);

#endif

private:

	wl_display* mWlDisplay;
	wl_registry* mWlRegistry;
	wl_registry_listener mWlRegistryListener;
	XenBackend::Log mLog;

	CompositorPtr mCompositor;
	ShellPtr mShell;
	SharedMemoryPtr mSharedMemory;

#ifdef WITH_IVI_EXTENSION
	IviApplicationPtr mIviApplication;
#endif

#ifdef WITH_INPUT
	SeatPtr mSeat;
#endif

#ifdef WITH_DRM
	WaylandDrmPtr mWaylandDrm;
	WaylandKmsPtr mWaylandKms;
#endif

	mutable std::mutex mMutex;
	std::thread mThread;

	std::unique_ptr<XenBackend::PollFd> mPollFd;

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

	void dispatchThread();
};

typedef std::shared_ptr<Display> DisplayPtr;

}

#endif /* SRC_WAYLAND_DISPLAY_HPP_ */
