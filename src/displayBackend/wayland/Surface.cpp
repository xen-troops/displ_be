/*
 * Surface.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include "Surface.hpp"

#include "Exception.hpp"

using DisplayItf::FrameBufferPtr;

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
	catch(const std::exception& e)
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
		throw Exception("Draw event is in progress", -EINVAL);
	}

	mStoredCallback = callback;

	if (mStoredCallback)
	{
		mWlFrameCallback = wl_surface_frame(mWlSurface);

		if (!mWlFrameCallback)
		{
			throw Exception("Can't get frame callback", -EINVAL);
		}

		if (wl_callback_add_listener(mWlFrameCallback,
									 &mWlFrameListener, this) < 0)
		{
			throw Exception("Can't add listener", -EINVAL);
		}
	}

	wl_surface_damage(mWlSurface, 0, 0,
					  frameBuffer->getWidth(), frameBuffer->getHeight());

	wl_surface_attach(mWlSurface,
					  reinterpret_cast<wl_buffer*>(frameBuffer->getHandle()),
					  0, 0);

	wl_surface_commit(mWlSurface);

	if (wl_display_flush(mWlDisplay) < 0)
	{
		throw Exception("Failed to flush display", -EINVAL);
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
		throw Exception("Can't create surface", -EINVAL);
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
