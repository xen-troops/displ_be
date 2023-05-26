/*
 * ShellSurface.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "ShellSurface.hpp"

#include "Exception.hpp"

namespace Wayland {

/*******************************************************************************
 * ShellSurface
 ******************************************************************************/

ShellSurface::ShellSurface(xdg_wm_base* shell, SurfacePtr surface) :
	mXDGSurface(nullptr),
	mSurface(surface),
	mLog("ShellSurface")
{
	try
	{
		init(shell);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

ShellSurface::~ShellSurface()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void ShellSurface::setTopLevel()
{
	LOG(mLog, DEBUG) << "Set top level";

	/*
	 * Creating an xdg_surface does not set the role for a wl_surface. In order
 	 * to map an xdg_surface, the client must create a role-specific object
 	 * using, e.g., get_toplevel, get_popup.
 	 */
	/* This creates an xdg_toplevel object for the given xdg_surface and gives
 	 * the associated wl_surface the xdg_toplevel role.
	*/
	mXDGToplevel = xdg_surface_get_toplevel(mXDGSurface);
	xdg_toplevel_set_title(mXDGToplevel, "Display Backend");
	wl_surface_commit(mSurface->mWlSurface);
}

/*******************************************************************************
 * Private
 ******************************************************************************/
void ShellSurface::sConfigHandler(void *data, xdg_surface *xdg_surface, uint32_t serial)
{
	static_cast<ShellSurface*>(data)->configHandler(xdg_surface, serial);
}

void ShellSurface::configHandler(xdg_surface *xdg_surface, uint32_t serial)
{
	DLOG(mLog, DEBUG) << "Config handler:" << serial;

	mSurface->handleSurfaceConfiguration(xdg_surface, serial);
}

void ShellSurface::init(xdg_wm_base* shell)
{
	mXDGSurface = xdg_wm_base_get_xdg_surface(shell, mSurface->mWlSurface);

	if (!mXDGSurface)
	{
		throw Exception("Can't create shell surface", errno);
	}

	mXDGSurfaceListener = {sConfigHandler};

	if (xdg_surface_add_listener(mXDGSurface, &mXDGSurfaceListener, this) < 0)
	{
		throw Exception("Can't add xdg_surface_add_listener", errno);
	}

	LOG(mLog, DEBUG) << "Create";
}

void ShellSurface::release()
{
	if (mXDGSurface)
	{
		xdg_surface_destroy(mXDGSurface);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
