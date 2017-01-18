/*
 * ShellSurface.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "ShellSurface.hpp"

#include "Exception.hpp"

using std::shared_ptr;

namespace Wayland {

/*******************************************************************************
 * ShellSurface
 ******************************************************************************/

ShellSurface::ShellSurface(wl_shell* shell, shared_ptr<Surface> surface) :
	mWlShellSurface(nullptr),
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

	wl_shell_surface_set_toplevel(mWlShellSurface);
}

void ShellSurface::setFullScreen()
{
	LOG(mLog, DEBUG) << "Set full screen";

	wl_shell_surface_set_fullscreen(mWlShellSurface,
			WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, nullptr);
}

/*******************************************************************************
 * Private
 ******************************************************************************/
void ShellSurface::sPingHandler(void *data, wl_shell_surface *shell_surface,
								uint32_t serial)
{
	static_cast<ShellSurface*>(data)->pingHandler(serial);
}

void ShellSurface::sConfigHandler(void *data, wl_shell_surface *shell_surface,
								  uint32_t edges, int32_t width, int32_t height)
{
	static_cast<ShellSurface*>(data)->configHandler(edges, width, height);
}

void ShellSurface::sPopupDone(void *data, wl_shell_surface *shell_surface)
{
	static_cast<ShellSurface*>(data)->popupDone();
}

void ShellSurface::pingHandler(uint32_t serial)
{
	DLOG(mLog, DEBUG) << "Ping handler: " << serial;

	wl_shell_surface_pong(mWlShellSurface, serial);
}

void ShellSurface::configHandler(uint32_t edges, int32_t width, int32_t height)
{
	DLOG(mLog, DEBUG) << "Config handler, edges: " << edges
					  << ", width: " << width << ", height: " << height;
}

void ShellSurface::popupDone()
{
	DLOG(mLog, DEBUG) << "Popup done";
}

void ShellSurface::init(wl_shell* shell)
{
	mWlShellSurface = wl_shell_get_shell_surface(shell, mSurface->mWlSurface);

	if (!mWlShellSurface)
	{
		throw Exception("Can't create shell surface");
	}

	mWlListener = {sPingHandler, sConfigHandler, sPopupDone};

	if (wl_shell_surface_add_listener(mWlShellSurface, &mWlListener, this) < 0)
	{
		throw Exception("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void ShellSurface::release()
{
	if (mWlShellSurface)
	{
		wl_shell_surface_destroy(mWlShellSurface);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
