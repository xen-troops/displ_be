/*
 * Shell.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Shell.hpp"

#include "Exception.hpp"

using std::shared_ptr;

namespace Wayland {

/*******************************************************************************
 * Shell
 ******************************************************************************/

Shell::Shell(wl_registry* registry, uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlShell(nullptr),
	mLog("Shell")
{
	try
	{
		init();
	}
	catch(const WlException& e)
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
shared_ptr<ShellSurface> Shell::createShellSurface(shared_ptr<Surface> surface)
{
	LOG(mLog, DEBUG) << "Create shell surface";

	return shared_ptr<ShellSurface>(new ShellSurface(mWlShell, surface));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Shell::init()
{
	mWlShell = static_cast<wl_shell*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_shell_interface, getVersion()));

	if (!mWlShell)
	{
		throw WlException("Can't bind shell");
	}

	LOG(mLog, DEBUG) << "Create";
}

void Shell::release()
{
	if (mWlShell)
	{
		wl_shell_destroy(mWlShell);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}

