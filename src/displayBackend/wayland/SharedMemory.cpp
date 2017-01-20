/*
 * SharedMemory.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "SharedMemory.hpp"

#include "Exception.hpp"

using DisplayItf::DisplayBufferPtr;
using DisplayItf::GrantRefs;

namespace Wayland {

/*******************************************************************************
 * SharedMemory
 ******************************************************************************/

SharedMemory::SharedMemory(wl_registry* registry, uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mWlSharedMemory(nullptr),
	mLog("SharedMemory")
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

SharedMemory::~SharedMemory()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

SharedFilePtr SharedMemory::createSharedFile(
		uint32_t width, uint32_t height, uint32_t bpp,
		domid_t domId, const GrantRefs& refs)
{
	LOG(mLog, DEBUG) << "Create shared file";

	return SharedFilePtr(new SharedFile(width, height, bpp, domId, refs));
}

SharedBufferPtr SharedMemory::createSharedBuffer(
		DisplayBufferPtr displayBuffer, uint32_t width, uint32_t height,
		uint32_t pixelFormat)
{
	LOG(mLog, DEBUG) << "Create shared buffer";

	return SharedBufferPtr(new SharedBuffer(mWlSharedMemory,
													 displayBuffer,
													 width, height,
													 pixelFormat));
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void SharedMemory::init()
{
	mWlSharedMemory = static_cast<wl_shm*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_shm_interface, getVersion()));

	if (!mWlSharedMemory)
	{
		throw Exception("Can't bind shared memory");
	}

	LOG(mLog, DEBUG) << "Create";
}

void SharedMemory::release()
{
	if (mWlSharedMemory)
	{
		wl_shm_destroy(mWlSharedMemory);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
