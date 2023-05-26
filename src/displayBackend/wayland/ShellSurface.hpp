/*
 * ShellSurface.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHELLSURFACE_HPP_
#define SRC_WAYLAND_SHELLSURFACE_HPP_

#include <memory>

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "Surface.hpp"

#include "xdg-shell-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * Wayland shell surface class.
 * @ingroup wayland
 ******************************************************************************/
class ShellSurface
{
public:

	~ShellSurface();

	/**
	 * Sets shell surface as top level
	 */
	void setTopLevel();

	/**
	 * Returns associated surface
	 */
	SurfacePtr getSurface() const { return mSurface; }

private:

	friend class Display;
	friend class Shell;

	ShellSurface(xdg_wm_base* shell, SurfacePtr surface);

	xdg_surface* mXDGSurface;
	SurfacePtr mSurface;
	xdg_surface_listener mXDGSurfaceListener;
	xdg_toplevel *mXDGToplevel;

	XenBackend::Log mLog;

	static void sConfigHandler(void *data, xdg_surface *xdg_surface, uint32_t serial);

	void configHandler(xdg_surface *xdg_surface, uint32_t serial);

	void init(xdg_wm_base* shell);
	void release();
};

typedef std::shared_ptr<ShellSurface> ShellSurfacePtr;

}

#endif /* SRC_WAYLAND_SHELLSURFACE_HPP_ */
