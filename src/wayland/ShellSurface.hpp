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

namespace Wayland {

class ShellSurface
{
public:

	~ShellSurface();

	void setTopLevel();

	std::shared_ptr<Surface> getSurface() const { return mSurfacePtr; }

public:

	ShellSurface(wl_shell* shell, std::shared_ptr<Surface> surface);

	wl_shell_surface* mShellSurface;
	std::shared_ptr<Surface> mSurfacePtr;
	wl_shell_surface_listener mListener;

	XenBackend::Log mLog;

	static void sPingHandler(void *data, wl_shell_surface *shell_surface,
							 uint32_t serial);
	static void sConfigHandler(void *data, wl_shell_surface *shell_surface,
							   uint32_t edges, int32_t width, int32_t height);
	static void sPopupDone(void *data, wl_shell_surface *shell_surface);

	void pingHandler(uint32_t serial);
	void configHandler(uint32_t edges, int32_t width, int32_t height);
	void popupDone();

	void init(wl_shell* shell);
	void release();
};

}

#endif /* SRC_WAYLAND_SHELLSURFACE_HPP_ */
