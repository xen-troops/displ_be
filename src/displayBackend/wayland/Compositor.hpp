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

#ifdef WITH_WAYLAND_PRESENTATION_FEEDBACK
#include "presentation-time-client-protocol.h"
#endif

namespace Wayland {

class Display;

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

#ifdef WITH_WAYLAND_PRESENTATION_FEEDBACK
	/**
	 * Pass pointer to wp_presentation into
	 * Compositor object
	 */
	void setPresentation(wp_presentation* p) {
		mWlPresentation = p;
	}
#endif

private:

	friend class Display;

	Compositor(wl_display* wlDisplay, wl_registry* registry,
			   uint32_t id, uint32_t version);

	wl_display* mWlDisplay;
	wl_compositor* mWlCompositor;
	XenBackend::Log mLog;
#ifdef WITH_WAYLAND_PRESENTATION_FEEDBACK
	wp_presentation* mWlPresentation{nullptr};
#endif

	void init();
	void release();
};

typedef std::shared_ptr<Compositor> CompositorPtr;

}

#endif /* SRC_WAYLAND_COMPOSITOR_HPP_ */
