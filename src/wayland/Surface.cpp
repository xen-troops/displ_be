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

Surface::Surface(wl_compositor* compositor) :
	mSurface(nullptr),
	mFrameCallback(nullptr),
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

void Surface::draw(std::shared_ptr<SharedBuffer> sharedBuffer,
				   FrameCallback callback)
{
	DLOG(mLog, DEBUG) << "Draw";

	mStoredCallback = callback;

	if (mFrameCallback)
	{
		throw WlException("Draw event is in progress");
	}

	mFrameCallback = wl_surface_frame(mSurface);

	if (!mFrameCallback)
	{
		throw WlException("Can't get frame callback");
	}

	if (wl_callback_add_listener(mFrameCallback, &mFrameListener, this) < 0)
	{
		throw WlException("Can't add listener");
	}

	wl_surface_damage(mSurface, 0, 0,
					  sharedBuffer->mWidth, sharedBuffer->mHeight);

	wl_surface_attach(mSurface, sharedBuffer->mBuffer, 0, 0);

	wl_surface_commit(mSurface);
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

	wl_callback_destroy(mFrameCallback);

	mFrameCallback = nullptr;

	if (mStoredCallback)
	{
		mStoredCallback();
	}
}

void Surface::init(wl_compositor* compositor)
{
	mSurface = wl_compositor_create_surface(compositor);

	if (!mSurface)
	{
		throw WlException("Can't create surface");
	}

	mFrameListener = { sFrameHandler };

	LOG(mLog, DEBUG) << "Create";
}

void Surface::release()
{
	if (mFrameCallback)
	{
		wl_callback_destroy(mFrameCallback);
	}

	if (mSurface)
	{
		wl_surface_destroy(mSurface);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
