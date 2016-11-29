/*
 * Compositor.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Compositor.hpp"

#include "Exception.hpp"

using std::shared_ptr;

namespace Wayland {

/*******************************************************************************
 * Compositor
 ******************************************************************************/

Compositor::Compositor(wl_registry* registry, uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mCompositor(nullptr),
	mLog("Compositor")
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

Compositor::~Compositor()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

shared_ptr<Surface> Compositor::createSurface()
{
	LOG(mLog, DEBUG) << "Create surface";

	return shared_ptr<Surface>(new Surface(mCompositor));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Compositor::init()
{
	mCompositor = static_cast<wl_compositor*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_compositor_interface, getVersion()));

	if (!mCompositor)
	{
		throw WlException("Can't bind compositor");
	}

	LOG(mLog, DEBUG) << "Create";
}

void Compositor::release()
{
	if (mCompositor)
	{
		wl_compositor_destroy(mCompositor);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}

