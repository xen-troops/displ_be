/*
 * IviApplication.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#include "IviApplication.hpp"

#include <ilm/ilm_client.h>

#include "Exception.hpp"

using std::shared_ptr;

namespace Wayland {

/*******************************************************************************
 * IviApplication
 ******************************************************************************/

IviApplication::IviApplication(wl_display* display) :
	mInitialised(false),
	mLog("IviApplication")
{
	try
	{
		init(display);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

IviApplication::~IviApplication()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

shared_ptr<IviSurface> IviApplication::createIviSurface(
		shared_ptr<Surface> surface,
		uint32_t width, uint32_t height,
		uint32_t pixelFormat)
{
	LOG(mLog, DEBUG) << "Create ivi surface";

	return shared_ptr<IviSurface>(new IviSurface(surface, width, height,
												 pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void IviApplication::init(wl_display* display)
{
	if (ilmClient_init(reinterpret_cast<t_ilm_nativedisplay>(display)) !=
		ILM_SUCCESS)
	{
		throw Exception("Can't initialize layer manage");
	}

	mInitialised = true;

	LOG(mLog, DEBUG) << "Create";
}

void IviApplication::release()
{
	if (mInitialised)
	{
		ilmClient_destroy();

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
