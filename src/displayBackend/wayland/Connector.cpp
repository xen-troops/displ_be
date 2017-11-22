/*
 *  Connector class
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

#include "Connector.hpp"
#include "Exception.hpp"
#include "SurfaceManager.hpp"

using DisplayItf::FrameBufferPtr;

namespace Wayland {

/*******************************************************************************
 * Connector
 ******************************************************************************/

Connector::Connector(const std::string& name, CompositorPtr compositor) :
	mCompositor(compositor),
	mName(name),
	mInitialized(false),
	mLog("Connector")
{
	LOG(mLog, DEBUG) << "Create, name: "  << mName;
}

Connector::~Connector()
{
	LOG(mLog, DEBUG) << "Delete, name: " << mName;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Connector::init(uint32_t width, uint32_t height,
					 FrameBufferPtr frameBuffer)
{
	onInit(mCompositor->createSurface(), frameBuffer);
}

void Connector::release()
{
	onRelease();
}

void Connector::pageFlip(FrameBufferPtr frameBuffer, FlipCallback cbk)
{
	DLOG(mLog, DEBUG) << "Page flip, name: " << mName;

	mSurface->draw(frameBuffer, cbk);
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void Connector::onInit(SurfacePtr surface, FrameBufferPtr frameBuffer)
{
	LOG(mLog, DEBUG) << "Init, name: " << mName;

	if (mInitialized)
	{
		throw Exception("Connector already initialized", -EINVAL);
	}

	mSurface = surface;

	SurfaceManager::getInstance().createSurface(mName, mSurface->mWlSurface);

	mSurface->draw(frameBuffer);

	mInitialized = true;
}

void Connector::onRelease()
{
	LOG(mLog, DEBUG) << "Release, name: " << mName;

	SurfaceManager::getInstance().deleteSurface(mName, mSurface->mWlSurface);

	mInitialized = false;

	mSurface.reset();
}

}
