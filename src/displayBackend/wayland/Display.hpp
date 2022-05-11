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
#ifdef WITH_ZCOPY
#include "WaylandZCopy.hpp"
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

	explicit Display(bool disable_zcopy = false);
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
	 * Flushes events
	 */
	void flush() override;

	/**
	 * Creates connector
	 * @param domId domaind ID
	 * @param name   connector name
	 * @param width  connector width as configured in XenStore
	 * @param height connector height as configured in XenStore
	 */
	DisplayItf::ConnectorPtr createConnector(domid_t domId,
											 const std::string& name,
											 uint32_t width,
											 uint32_t height) override;

	/**
	 * Creates display buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @param offset offset of the data in the buffer
	 * @return shared pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp,
			size_t offset) override;

	/**
	 * Creates display buffer with associated grant table buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @param offset offset of the data in the buffer
	 * @return shared pointer to the display buffer
	 */
	DisplayItf::DisplayBufferPtr createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp, size_t offset,
			domid_t domId, GrantRefs& refs,
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
	bool mDisableZCopy;
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

#ifdef WITH_ZCOPY
#ifdef WITH_DRM_ZCOPY
	WaylandDrmPtr mWaylandDrm;
#endif
#ifdef WITH_KMS_ZCOPY
	WaylandKmsPtr mWaylandKms;
#endif
#ifdef WITH_DMABUF_ZCOPY
	WaylandLinuxDmabufPtr mWaylandLinuxDmabuf;
#endif
#endif

	mutable std::mutex mMutex;
	std::thread mThread;

	std::unique_ptr<XenBackend::PollFd> mPollFd;

	static void sWaylandLog(const char* fmt, va_list arg);

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

	int checkWaylandError();
	void dispatchThread();
};

typedef std::shared_ptr<Display> DisplayPtr;

}

#endif /* SRC_WAYLAND_DISPLAY_HPP_ */
