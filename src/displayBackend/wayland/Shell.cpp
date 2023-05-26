/*
 * Shell.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Shell.hpp"

#include "Exception.hpp"

namespace Wayland {

/*******************************************************************************
 * Shell
 ******************************************************************************/

Shell::Shell(wl_registry* registry, uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mXDGShell(nullptr),
	mLog("Shell")
{
	try
	{
		init();
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

Shell::~Shell()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/
ShellSurfacePtr Shell::createShellSurface(SurfacePtr surface)
{
	LOG(mLog, DEBUG) << "Create shell surface";

	return ShellSurfacePtr(new ShellSurface(mXDGShell, surface));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Shell::init()
{
	mXDGShell = bind<xdg_wm_base*>(&xdg_wm_base_interface);

	if (!mXDGShell)
	{
		throw Exception("Can't bind shell", errno);
	}

	mWlListener = {sPingHandler};

	if (xdg_wm_base_add_listener(mXDGShell, &mWlListener, this) < 0)
	{
		throw Exception("Can't add xdg_wm_base_add_listener", errno);
	}

	LOG(mLog, DEBUG) << "Create";
}

void Shell::sPingHandler(void *data, xdg_wm_base *shell_surface,
								uint32_t serial)
{
	xdg_wm_base_pong(shell_surface, serial);
}

void Shell::release()
{
	if (mXDGShell)
	{
		xdg_wm_base_destroy(mXDGShell);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}

