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

using DisplayItf::FrameBufferPtr;

namespace Wayland {

/*******************************************************************************
 * Connector
 ******************************************************************************/

Connector::Connector(const std::string& name, SurfacePtr surface) :
	mName(name),
	mSurface(surface),
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

void Connector::init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
					 FrameBufferPtr frameBuffer)
{
	LOG(mLog, DEBUG) << "Init, name: " << mName << ", x: " << x << ", y: " << y
					 << ", w: " << width << ", height: " << height;

	mSurface->draw(frameBuffer);

	mInitialized = true;
}

void Connector::release()
{
	LOG(mLog, DEBUG) << "Release, name: " << mName;

	mInitialized = false;
}

void Connector::pageFlip(FrameBufferPtr frameBuffer, FlipCallback cbk)
{
	DLOG(mLog, DEBUG) << "Page flip, name: " << mName;

	mSurface->draw(frameBuffer, cbk);
}

}
