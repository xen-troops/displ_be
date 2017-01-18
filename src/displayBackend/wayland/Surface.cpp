/*
 * Surface.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Surface.hpp"

#include "Exception.hpp"

namespace Wayland {

/*******************************************************************************
 * Surface
 ******************************************************************************/

Surface::Surface(wl_display* display, wl_compositor* compositor) :
	mWlDisplay(display),
	mWlSurface(nullptr),
	mWlFrameCallback(nullptr),
	mLog("Surface")
{
	try
	{
		init(compositor);
	}
	catch(const WlException& e)
	{
		release();

		throw;
	}
}

Surface::~Surface()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Surface::draw(FrameBufferPtr frameBuffer,
				   FrameCallback callback)
{
	DLOG(mLog, DEBUG) << "Draw";

	if (mWlFrameCallback)
	{
		throw WlException("Draw event is in progress");
	}

	mStoredCallback = callback;

	if (mStoredCallback)
	{
		mWlFrameCallback = wl_surface_frame(mWlSurface);

		if (!mWlFrameCallback)
		{
			throw WlException("Can't get frame callback");
		}

		if (wl_callback_add_listener(mWlFrameCallback, &mWlFrameListener, this) < 0)
		{
			throw WlException("Can't add listener");
		}
	}

	wl_surface_damage(mWlSurface, 0, 0,
					  frameBuffer->getWidth(), frameBuffer->getHeight());

	wl_surface_attach(mWlSurface,
					  reinterpret_cast<wl_buffer*>(frameBuffer->getHandle()),
					  0, 0);

	wl_surface_commit(mWlSurface);

	// as above wl commands are added async we have to push wl

	if (wl_display_dispatch_pending(mWlDisplay) == -1)
	{
		throw WlException("Failed to dispatch pending events");
	}

	if (wl_display_flush(mWlDisplay) == -1)
	{
		throw WlException("Failed to flush display");
	}
}

/*******************************************************************************
 * Private
 ******************************************************************************/
void Surface::sFrameHandler(void *data, wl_callback *wl_callback,
							uint32_t callback_data)
{
	static_cast<Surface*>(data)->frameHandler();
}

void Surface::frameHandler()
{
	DLOG(mLog, DEBUG) << "Frame handler";

	wl_callback_destroy(mWlFrameCallback);

	mWlFrameCallback = nullptr;

	if (mStoredCallback)
	{
		mStoredCallback();
	}
}

void Surface::init(wl_compositor* compositor)
{
	mWlSurface = wl_compositor_create_surface(compositor);

	if (!mWlSurface)
	{
		throw WlException("Can't create surface");
	}

	mWlFrameListener = { sFrameHandler };

	LOG(mLog, DEBUG) << "Create: " << mWlSurface;
}

void Surface::release()
{
	if (mWlFrameCallback)
	{
		wl_callback_destroy(mWlFrameCallback);
	}

	if (mWlSurface)
	{
		wl_surface_destroy(mWlSurface);

		LOG(mLog, DEBUG) << "Delete: " << mWlSurface;
	}
}

}
