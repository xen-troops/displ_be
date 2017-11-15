/*
 *  Display class
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 *
 */

#include "Display.hpp"

#include "Exception.hpp"

using namespace std::placeholders;

using std::bind;
using std::exception;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;

using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;
using DisplayItf::GrantRefs;

#ifdef WITH_INPUT
using InputItf::KeyboardCallbacks;
using InputItf::PointerCallbacks;
using InputItf::TouchCallbacks;
#endif

using XenBackend::PollFd;

namespace Wayland {

/*******************************************************************************
 * Display
 ******************************************************************************/

Display::Display() :
	mWlDisplay(nullptr),
	mWlRegistry(nullptr),
	mLog("Display")
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

Display::~Display()
{
	stop();

	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Display::start()
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Start";

	mThread = thread(&Display::dispatchThread, this);
}

void Display::stop()
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Stop";

	if (mPollFd)
	{
		mPollFd->stop();
	}

	if (mThread.joinable())
	{
		mThread.join();
	}
}

bool Display::isZeroCopySupported() const
{
	lock_guard<mutex> lock(mMutex);

#ifdef WITH_DRM
	if (mWaylandDrm && mWaylandDrm->isZeroCopySupported())
	{
		return true;
	}
#endif
	return false;
}

DisplayItf::ConnectorPtr Display::createConnector(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	Connector* connector = nullptr;

	if (mShell)
	{
		connector = new ShellConnector(name, mShell,
									   mCompositor->createSurface());

		LOG(mLog, DEBUG) << "Create shell connector, name: " << name;
	}
#ifdef WITH_IVI_EXTENSION
	else if (mIviApplication)
	{
		uint32_t surfaceId = 0;

		try
		{
			surfaceId = stoi(name);
		}
		catch(const exception& e)
		{
			throw Exception("Can't create surface id: " + name);
		}

		connector = new IviConnector(name, mIviApplication,
									 mCompositor->createSurface(), surfaceId);

		LOG(mLog, DEBUG) << "Create ivi connector, name: " << name;
	}
#endif
	else
	{
		connector = new Connector(name, mCompositor->createSurface());

		LOG(mLog, DEBUG) << "Create connector, name: " << name;
	}

	Wayland::ConnectorPtr connectorPtr(connector);

	return connectorPtr;
}

DisplayBufferPtr Display::createDisplayBuffer(
		uint32_t width, uint32_t height, uint32_t bpp)
{
	lock_guard<mutex> lock(mMutex);

	if (mSharedMemory)
	{
		return mSharedMemory->createSharedFile(width, height, bpp);
	}

	throw Exception("Can't create display buffer");
}

DisplayBufferPtr Display::createDisplayBuffer(
		uint32_t width, uint32_t height, uint32_t bpp,
		domid_t domId, GrantRefs& refs, bool allocRefs)
{
	lock_guard<mutex> lock(mMutex);

#ifdef WITH_DRM
	if (mWaylandDrm)
	{
		return mWaylandDrm->createDumb(width, height, bpp,
									   domId, refs, allocRefs);
	}
	else
#endif
	if (mSharedMemory)
	{
		return mSharedMemory->createSharedFile(width, height, bpp, domId, refs);
	}

	throw Exception("Can't create display buffer");
}

FrameBufferPtr Display::createFrameBuffer(DisplayBufferPtr displayBuffer,
										  uint32_t width, uint32_t height,
										  uint32_t pixelFormat)
{
	lock_guard<mutex> lock(mMutex);

#ifdef WITH_DRM
	if (mWaylandDrm)
	{
		return mWaylandDrm->createDrmBuffer(displayBuffer, width, height,
											pixelFormat);
	}
	else
#endif
	if (mSharedMemory)
	{
		return mSharedMemory->createSharedBuffer(displayBuffer, width, height,
												 pixelFormat);
	}

	throw Exception("Can't create frame buffer");
}


#ifdef WITH_INPUT

template<>
void Display::setInputCallbacks<KeyboardCallbacks>(
		const string& connector, const KeyboardCallbacks& callbacks)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Set keyboard callback for connector: " << connector;

	if (mSeat)
	{
		auto keyboard = mSeat->getKeyboard();

		if (keyboard)
		{
			keyboard->setConnectorCallbacks(connector, callbacks);
		}
		else
		{
			LOG(mLog, WARNING) << "No keyboard for input callbacks";
		}
	}
	else
	{
		LOG(mLog, WARNING) << "No seat for input callbacks";
	}
}

template<>
void Display::clearInputCallbacks<KeyboardCallbacks>(const string& connector)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Clear keyboard callback for connector: " << connector;

	if (mSeat)
	{
		auto keyboard = mSeat->getKeyboard();

		if (keyboard)
		{
			keyboard->clearConnectorCallbacks(connector);
		}
	}
}

template<>
void Display::setInputCallbacks<PointerCallbacks>(
		const string& connector, const PointerCallbacks& callbacks)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Set pointer callback for connector: " << connector;

	if (mSeat)
	{
		auto pointer = mSeat->getPointer();

		if (pointer)
		{
			pointer->setConnectorCallbacks(connector, callbacks);
		}
		else
		{
			LOG(mLog, WARNING) << "No pointer for input callbacks";
		}
	}
	else
	{
		LOG(mLog, WARNING) << "No seat for input callbacks";
	}
}

template<>
void Display::clearInputCallbacks<PointerCallbacks>(const string& connector)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Clear pointer callback for connector: " << connector;

	if (mSeat)
	{
		auto pointer = mSeat->getPointer();

		if (pointer)
		{
			pointer->clearConnectorCallbacks(connector);
		}
	}
}

template<>
void Display::setInputCallbacks<TouchCallbacks>(
		const string& connector, const TouchCallbacks& callbacks)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Set touch callback for connector: " << connector;

	if (mSeat)
	{
		auto touch = mSeat->getTouch();

		if (touch)
		{
			touch->setConnectorCallbacks(connector, callbacks);
		}
		else
		{
			LOG(mLog, WARNING) << "No touch for input callbacks";
		}
	}
	else
	{
		LOG(mLog, WARNING) << "No seat for input callbacks";
	}
}

template<>
void Display::clearInputCallbacks<TouchCallbacks>(const string& connector)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Clear touch callback for connector: " << connector;

	if (mSeat)
	{
		auto touch = mSeat->getTouch();

		if (touch)
		{
			touch->clearConnectorCallbacks(connector);
		}
	}
}

#endif
/*******************************************************************************
 * Private
 ******************************************************************************/

void Display::sRegistryHandler(void *data, wl_registry *registry, uint32_t id,
							   const char *interface, uint32_t version)
{
	static_cast<Display*>(data)->registryHandler(registry, id, interface,
												 version);
}

void Display::sRegistryRemover(void *data, struct wl_registry *registry,
							   uint32_t id)
{
	static_cast<Display*>(data)->registryRemover(registry, id);
}

void Display::registryHandler(wl_registry *registry, uint32_t id,
							  const std::string& interface, uint32_t version)
{
	LOG(mLog, DEBUG) << "Registry event, itf: " << interface << ", id: " << id
					 << ", version: " << version;

	if (interface == "wl_compositor")
	{
		mCompositor.reset(new Compositor(mWlDisplay, registry, id, version));
	}

	if (interface == "wl_shell")
	{
		mShell.reset(new Shell(registry, id, version));
	}

	if (interface == "wl_shm")
	{
		mSharedMemory.reset(new SharedMemory(registry, id, version));
	}
#ifdef WITH_IVI_EXTENSION
	if (interface == "ivi_application")
	{
		mIviApplication.reset(new IviApplication(registry, id, version));
	}
#endif
#ifdef WITH_INPUT
	if (interface == "wl_seat")
	{
		mSeat.reset(new Seat(registry, id, version));
	}
#endif
#ifdef WITH_DRM
	if (interface == "wl_drm")
	{
		mWaylandDrm.reset(new WaylandDrm(registry, id, version));
	}
#endif
}

void Display::registryRemover(wl_registry *registry, uint32_t id)
{
	LOG(mLog, DEBUG) << "Registry removed event, id: " << id;
}

void Display::init()
{
	mWlDisplay = wl_display_connect(nullptr);

	if (!mWlDisplay)
	{
		throw Exception("Can't connect to display");
	}

	mPollFd.reset(new PollFd(wl_display_get_fd(mWlDisplay), POLLIN | POLLOUT));

	LOG(mLog, DEBUG) << "Connected";

	mWlRegistryListener = {sRegistryHandler, sRegistryRemover};

	mWlRegistry = wl_display_get_registry(mWlDisplay);

	if (!mWlRegistry)
	{
		throw Exception("Can't get registry");
	}

	wl_registry_add_listener(mWlRegistry, &mWlRegistryListener, this);

	wl_display_dispatch(mWlDisplay);
	wl_display_roundtrip(mWlDisplay);

	if (!mCompositor)
	{
		throw Exception("Can't get compositor");
	}

	if (!mSharedMemory)
	{
		throw Exception("Can't get shared memory");
	}
}

void Display::release()
{
	// clear connectors first as it keeps Surfaces which should be deleted
	// prior IviApplication

#ifdef WITH_IVI_EXTENSION
	mIviApplication.reset();
#endif
	mShell.reset();
	mSharedMemory.reset();
	mCompositor.reset();
#ifdef WITH_INPUT
	mSeat.reset();
#endif
#ifdef WITH_DRM
	mWaylandDrm.reset();
#endif

	if (mWlRegistry)
	{
		wl_registry_destroy(mWlRegistry);
	}

	if (mWlDisplay)
	{
		wl_display_flush(mWlDisplay);
		wl_display_disconnect(mWlDisplay);

		LOG(mLog, DEBUG) << "Disconnected";
	}
}

void Display::dispatchThread()
{
	try
	{
		bool terminate = false;

		while(!terminate)
		{
			while (wl_display_prepare_read(mWlDisplay) != 0)
			{
				auto val = wl_display_dispatch_pending(mWlDisplay);

				if (val < 0)
				{
					throw Exception("Can't dispatch pending events");
				}

				DLOG(mLog, DEBUG) << "Dispatch events: " << val;
			}

			wl_display_flush(mWlDisplay);

			if (mPollFd->poll())
			{
				wl_display_read_events(mWlDisplay);
			}
			else
			{
				terminate = true;
			}
		}
	}
	catch(const exception& e)
	{
		LOG(mLog, ERROR) << e.what();
	}

	wl_display_cancel_read(mWlDisplay);
}

}
