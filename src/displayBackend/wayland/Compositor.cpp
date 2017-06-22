/*
 * Compositor.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Compositor.hpp"

#include "Exception.hpp"

namespace Wayland {

/*******************************************************************************
 * Compositor
 ******************************************************************************/

Compositor::Compositor(wl_display* display, wl_registry* registry,
					  uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlDisplay(display),
	mWlCompositor(nullptr),
	mLog("Compositor")
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

Compositor::~Compositor()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

SurfacePtr Compositor::createSurface()
{
	LOG(mLog, DEBUG) << "Create surface";

	return SurfacePtr(new Surface(mWlDisplay, mWlCompositor));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Compositor::init()
{
	mWlCompositor = static_cast<wl_compositor*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_compositor_interface, getVersion()));

	if (!mWlCompositor)
	{
		throw Exception("Can't bind compositor");
	}

	LOG(mLog, DEBUG) << "Create";
}

void Compositor::release()
{
	if (mWlCompositor)
	{
		wl_compositor_destroy(mWlCompositor);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}

