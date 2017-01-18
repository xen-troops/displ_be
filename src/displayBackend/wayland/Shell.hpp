/*
 * Shell.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SHELL_HPP_
#define SRC_WAYLAND_SHELL_HPP_

#include <xen/be/Log.hpp>

#include "Registry.hpp"
#include "ShellSurface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland shell class.
 * @ingroup wayland
 ******************************************************************************/
class Shell : public Registry
{
public:

	~Shell();

	/**
	 * Creates shell surface
	 * @param surface surface
	 */
	ShellSurfacePtr createShellSurface(SurfacePtr surface);

private:

	friend class Display;

	Shell(wl_registry* registry, uint32_t id, uint32_t version);

	wl_shell* mWlShell;
	XenBackend::Log mLog;

	void init();
	void release();
};

typedef std::shared_ptr<Shell> ShellPtr;

}

#endif /* SRC_WAYLAND_SHELL_HPP_ */
