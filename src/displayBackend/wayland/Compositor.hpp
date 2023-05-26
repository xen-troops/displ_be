/*
 * Compositor.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_COMPOSITOR_HPP_
#define SRC_WAYLAND_COMPOSITOR_HPP_

#include <memory>

#include <xen/be/Log.hpp>

#include "Registry.hpp"
#include "Surface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland compositor class.
 * @ingroup wayland
 ******************************************************************************/
class Compositor : public Registry
{
public:

	~Compositor();

	/**
	 * Creates surface
	 */
	SurfacePtr createSurface();

	/**
	 * Blocks until the compositor process all currently issued requests and sends
	 * out pending events on all event queues.
	 */
	void displayRpundtrip();

private:

	friend class Display;

	Compositor(wl_display* display, wl_registry* registry,
			   uint32_t id, uint32_t version);

	wl_display* mWlDisplay;
	wl_compositor* mWlCompositor;
	XenBackend::Log mLog;

	void init();
	void release();
};

typedef std::shared_ptr<Compositor> CompositorPtr;

}

#endif /* SRC_WAYLAND_COMPOSITOR_HPP_ */
