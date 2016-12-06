/*
 * Surface.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SURFACE_HPP_
#define SRC_WAYLAND_SURFACE_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "SharedBuffer.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland surface class.
 * @ingroup wayland
 ******************************************************************************/
class Surface
{
public:

	/**
	 * Callback which is called when frame is displayed
	 */
	typedef std::function<void()> FrameCallback;

	~Surface();

	/**
	 * Draws shared buffer content
	 * @param sharedBuffer shared buffer
	 * @param callback     called when frame with the content is displayed
	 */
	void draw(std::shared_ptr<SharedBuffer> sharedBuffer,
			  FrameCallback callback = nullptr);

private:

	friend class Display;
	friend class ShellSurface;
	friend class Compositor;

	Surface(wl_display* display, wl_compositor* compositor);

	wl_display* mDisplay;
	wl_surface* mSurface;
	wl_callback *mFrameCallback;
	XenBackend::Log mLog;

	wl_callback_listener mFrameListener;

	FrameCallback mStoredCallback;

	static void sFrameHandler(void *data, wl_callback *wl_callback,
							  uint32_t callback_data);
	void frameHandler();

	void init(wl_compositor* compositor);
	void release();
};

}

#endif /* SRC_WAYLAND_SURFACE_HPP_ */
