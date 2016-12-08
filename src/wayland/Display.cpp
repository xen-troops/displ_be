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

#include <drm_fourcc.h>

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
	mWlDisplay(nullptr),
	mWlRegistry(nullptr),
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

void Display::createBackgroundSurface(uint32_t width, uint32_t height)
{
	if (mShell)
	{
		LOG(mLog, DEBUG) << "Create background surface, w: " << width
						 << ", h: " << height;

		mBackgroundSurface = mShell->createShellSurface(
				mCompositor->createSurface());

		mBackgroundSurface->setFullScreen();

		auto sharedFile = mSharedMemory->createSharedFile(width, height, 32);
		auto sharedBuffer = mSharedMemory->createSharedBuffer(
							sharedFile, width, height, DRM_FORMAT_XRGB8888);

		mBackgroundSurface->mSurface->draw(sharedBuffer);
	}
	else
	{
		LOG(mLog, WARNING) << "Can't create background surface";
	}
}

shared_ptr<ConnectorItf> Display::createConnector(uint32_t id, uint32_t x,
												  uint32_t y, uint32_t width,
												  uint32_t height)
{

	Connector* connector = nullptr;

	if (mShell)
	{
		connector = new ConnectorType<ShellSurface>(id, createShellSurface(x, y));
	}
	else if (mIviApplication)
	{
		connector = new ConnectorType<IviSurface>(id, createIviSurface(x, y, width, height));
	}
	else
	{
		connector = new Connector(id, mCompositor->createSurface());
	}

	LOG(mLog, DEBUG) << "Create connector, id: " << id;

	mConnectors.emplace(id, shared_ptr<Connector>(connector));

	return getConnectorById(id);
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

shared_ptr<ShellSurface> Display::createShellSurface(uint32_t x, uint32_t y)
{
	auto shellSurface =
			mShell->createShellSurface(mCompositor->createSurface());

	if (mBackgroundSurface)
	{
		LOG(mLog, DEBUG) << "Create child surface";

		wl_shell_surface_set_transient(shellSurface->mWlShellSurface,
				mBackgroundSurface->mSurface->mWlSurface,
				x, y, WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
	}
	else
	{
		LOG(mLog, DEBUG) << "Create toplevel surface";

		shellSurface->setTopLevel();
	}

	return shellSurface;
}

shared_ptr<IviSurface> Display::createIviSurface(uint32_t x, uint32_t y,
												 uint32_t width,
												 uint32_t height)
{
	return mIviApplication->createIviSurface(mCompositor->createSurface(),
											 width, height, 0);
}

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

	if (interface == "ivi_application")
	{
		mIviApplication.reset(new IviApplication(mWlDisplay));
	}
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
		throw WlException("Can't connect to display");
	}

	LOG(mLog, DEBUG) << "Connected";

	mWlRegistryListener = {sRegistryHandler, sRegistryRemover};

	mWlRegistry = wl_display_get_registry(mWlDisplay);

	if (!mWlRegistry)
	{
		throw WlException("Can't get registry");
	}

	wl_registry_add_listener(mWlRegistry, &mWlRegistryListener, this);

	wl_display_dispatch(mWlDisplay);
	wl_display_roundtrip(mWlDisplay);

	if (!mCompositor)
	{
		throw WlException("Can't get compositor");
	}

	if (!mSharedMemory)
	{
		throw WlException("Can't get shared memory");
	}
}

void Display::release()
{
	// clear connectors first as it keeps Surfaces which should be deleted
	// prior IviApplication

	mConnectors.clear();

	mIviApplication.reset();
	mShell.reset();

	mSharedMemory.reset();

	mCompositor.reset();

	std::shared_ptr<Shell> mShell;
	std::shared_ptr<SharedMemory> mSharedMemory;
	std::shared_ptr<IviApplication> mIviApplication;

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

bool Display::pollDisplayFd()
{
	pollfd fds;

	fds.fd = wl_display_get_fd(mWlDisplay);
	fds.events = POLLIN;

	while(!mTerminate)
	{
		fds.revents = 0;

		wl_display_flush(mWlDisplay);

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
			while (wl_display_prepare_read(mWlDisplay) != 0)
			{
				auto val = wl_display_dispatch_pending(mWlDisplay);

				if (val < 0)
				{
					throw WlException("Can't dispatch pending events");
				}

				DLOG(mLog, DEBUG) << "Dispatch events: " << val;
			}

			if (pollDisplayFd())
			{
				wl_display_read_events(mWlDisplay);
			}
			else
			{
				wl_display_cancel_read(mWlDisplay);
			}
		}
	}
	catch(const exception& e)
	{
		wl_display_cancel_read(mWlDisplay);

		LOG(mLog, ERROR) << e.what();
	}
}

}
