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
	wl_compositor* getCompositor() const {
		return mWlCompositor;
	}

#ifdef WITH_WAYLAND_PRESENTATION_API
	void setPresentation(wp_presentation* p) {
		mWlPresentation = p;
	}

	wp_presentation* getPresentation() const {
		return mWlPresentation;
	}
#endif

private:

	friend class Display;

	Compositor(wl_display* display, wl_registry* registry,
			   uint32_t id, uint32_t version);

	wl_display* mWlDisplay;
	wl_compositor* mWlCompositor{nullptr};
#ifdef WITH_WAYLAND_PRESENTATION_API
	wp_presentation* mWlPresentation{nullptr};
#endif
	XenBackend::Log mLog;

	void init();
	void release();
};

typedef std::shared_ptr<Compositor> CompositorPtr;

}

#endif /* SRC_WAYLAND_COMPOSITOR_HPP_ */
