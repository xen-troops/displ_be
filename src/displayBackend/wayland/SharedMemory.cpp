/*
 * SharedMemory.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "SharedMemory.hpp"

#include "Exception.hpp"

using std::hex;
using std::setfill;
using std::setw;

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

void SharedMemory::sFormatHandler(void *data, wl_shm *wlShm, uint32_t format)
{
	static_cast<SharedMemory*>(data)->formatHandler(format);
}

void SharedMemory::formatHandler(uint32_t format)
{
	LOG(mLog, DEBUG) << "Format: 0x" << hex << setfill('0') << setw(8)
					 << format;
}

void SharedMemory::init()
{
	mWlSharedMemory = static_cast<wl_shm*>(bind(&wl_shm_interface));

	if (!mWlSharedMemory)
	{
		throw Exception("Can't bind shared memory", errno);
	}

	mWlListener = {sFormatHandler};

	if (wl_shm_add_listener(mWlSharedMemory, &mWlListener, this) < 0)
	{
		throw Exception("Can't add listener", errno);
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
