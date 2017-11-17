/*
 * IviApplication.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#include "IviApplication.hpp"

#include <ilm/ilm_client.h>

#include "Exception.hpp"

namespace Wayland {

/*******************************************************************************
 * IviApplication
 ******************************************************************************/

IviApplication::IviApplication(wl_registry* registry,
							   uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlIviApplication(nullptr),
	mLog("IviApplication")
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

IviApplication::~IviApplication()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

IviSurfacePtr IviApplication::createIviSurface(SurfacePtr surface,
											   uint32_t surfaceId)
{
	LOG(mLog, DEBUG) << "Create ivi surface, id: " << surfaceId;

	return IviSurfacePtr(new IviSurface(mWlIviApplication, surface, surfaceId));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void IviApplication::init()
{
	mWlIviApplication = static_cast<ivi_application*>(
			wl_registry_bind(getRegistry(), getId(),
							 &ivi_application_interface, getVersion()));

	if (!mWlIviApplication)
	{
		throw Exception("Can't bind IVI application", -EINVAL);
	}

	LOG(mLog, DEBUG) << "Create";

}

void IviApplication::release()
{
	if (mWlIviApplication)
	{
		ivi_application_destroy(mWlIviApplication);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
