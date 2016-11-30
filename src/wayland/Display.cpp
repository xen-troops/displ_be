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

#include <poll.h>

#include "Exception.hpp"

using namespace std::placeholders;

using std::bind;
using std::dynamic_pointer_cast;
using std::exception;
using std::shared_ptr;
using std::to_string;
using std::thread;

namespace Wayland {

/*******************************************************************************
 * Display
 ******************************************************************************/

Display::Display() :
	mDisplay(nullptr),
	mRegistry(nullptr),
	mTerminate(false),
	mLog("Display")
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

Display::~Display()
{
	stop();

	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/
void Display::createConnector(uint32_t id, uint32_t x, uint32_t y,
							  uint32_t width, uint32_t height)
{
	LOG(mLog, DEBUG) << "Create connector, id: " << id;

	auto shellSurface = mShell->getShellSurface(mCompositor->createSurface());


	wl_shell_surface_set_transient(shellSurface->mShellSurface,
								   mMainShellSurface->getSurface()->mSurface,
								   x, y, WL_SHELL_SURFACE_TRANSIENT_INACTIVE);

//	shellSurface->setTopLevel();

	auto connector = new Connector(shellSurface, id, x, y, width, height);

	mConnectors.emplace(id, shared_ptr<Connector>(connector));
}

void Display::start()
{
	LOG(mLog, DEBUG) << "Start";

	mTerminate = false;

	mThread = thread(&Display::dispatchThread, this);
}

void Display::stop()
{
	LOG(mLog, DEBUG) << "Stop";

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

shared_ptr<ConnectorItf> Display::getConnectorById(uint32_t id)
{
	auto iter = mConnectors.find(id);

	if (iter == mConnectors.end())
	{
		throw WlException("Wrong connector id " + to_string(id));
	}

	return dynamic_pointer_cast<ConnectorItf>(iter->second);
}

shared_ptr<DisplayBufferItf> Display::createDisplayBuffer(uint32_t width,
														  uint32_t height,
														  uint32_t bpp)
{
	return mSharedMemory->createSharedFile(width, height, bpp);
}

shared_ptr<FrameBufferItf> Display::createFrameBuffer(
		shared_ptr<DisplayBufferItf> displayBuffer,
		uint32_t width, uint32_t height, uint32_t pixelFormat)
{
	return mSharedMemory->createSharedBuffer(
			dynamic_pointer_cast<SharedFile>(displayBuffer), width,
			height, pixelFormat);
}

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
		mCompositor.reset(new Compositor(registry, id, version));
	}

	if (interface == "wl_shell")
	{
		mShell.reset(new Shell(registry, id, version));
	}

	if (interface == "wl_shm")
	{
		mSharedMemory.reset(new SharedMemory(registry, id, version));
	}
}

void Display::registryRemover(wl_registry *registry, uint32_t id)
{
	LOG(mLog, DEBUG) << "Registry removed event, id: " << id;
}

void Display::mainShellSurfaceConfigCbk(uint32_t edges, int32_t width,
										int32_t height)
{
	LOG(mLog, DEBUG) << "Main shell surface config, w: " << width
					 << ", h: " << height;

	auto file = mSharedMemory->createSharedFile(width, height, 32);

	mMainSharedBuffer =
			mSharedMemory->createSharedBuffer(file, width, height,
											  WL_SHM_FORMAT_XRGB8888);

	mMainShellSurface->getSurface()->draw(mMainSharedBuffer);
}

void Display::init()
{
	mDisplay = wl_display_connect(nullptr);

	if (!mDisplay)
	{
		throw WlException("Can't connect to display");
	}

	LOG(mLog, DEBUG) << "Connected";

	mRegistryListener = {sRegistryHandler, sRegistryRemover};

	mRegistry = wl_display_get_registry(mDisplay);

	if (!mRegistry)
	{
		throw WlException("Can't get registry");
	}

	wl_registry_add_listener(mRegistry, &mRegistryListener, this);

	wl_display_dispatch(mDisplay);
	wl_display_roundtrip(mDisplay);

	if (!mCompositor)
	{
		throw WlException("Can't get compositor");
	}

	if (!mShell)
	{
		throw WlException("Can't get shell");
	}

	if (!mSharedMemory)
	{
		throw WlException("Can't get shared memory");
	}


	mMainShellSurface = mShell->getShellSurface(mCompositor->createSurface());

	mMainShellSurface->setConfigCallback(
			bind(&Display::mainShellSurfaceConfigCbk, this, _1, _2, _3));

	mMainShellSurface->setFullScreen();
}

void Display::release()
{
	if (mRegistry)
	{
		wl_registry_destroy(mRegistry);
	}

	if (mDisplay)
	{
		wl_display_flush(mDisplay);
		wl_display_disconnect(mDisplay);

		LOG(mLog, DEBUG) << "Disconnected";
	}
}

bool Display::pollDisplayFd()
{
	pollfd fds;

	fds.fd = wl_display_get_fd(mDisplay);
	fds.events = POLLIN;

	while(!mTerminate)
	{
		fds.revents = 0;

		wl_display_flush(mDisplay);

		auto ret = poll(&fds, 1, cPoolEventTimeoutMs);

		if (ret < 0)
		{
			throw WlException("Can't poll events");
		}
		else if (ret > 0)
		{
			return true;
		}
	}

	return false;
}

void Display::dispatchThread()
{
	try
	{
		while(!mTerminate)
		{
			while (wl_display_prepare_read(mDisplay) != 0)
			{
				auto val = wl_display_dispatch_pending(mDisplay);

				if (val < 0)
				{
					throw WlException("Can't dispatch pending events");
				}

				DLOG(mLog, DEBUG) << "Dispatch events: " << val;
			}

			if (pollDisplayFd())
			{
				wl_display_read_events(mDisplay);
			}
			else
			{
				wl_display_cancel_read(mDisplay);
			}
		}
	}
	catch(const exception& e)
	{
		wl_display_cancel_read(mDisplay);

		LOG(mLog, ERROR) << e.what();
	}
}

}
