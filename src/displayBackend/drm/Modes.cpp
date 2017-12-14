/*
 *  Mode classes
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

#include "Modes.hpp"

#include <xen/be/Log.hpp>

#include "Exception.hpp"

using std::to_string;

namespace Drm {

/*******************************************************************************
 * ModeResource
 ******************************************************************************/

ModeResource::ModeResource(int fd)
{
	DLOG("ModeResource", DEBUG) << "Create";

	mData = drmModeGetResources(fd);

	if (!mData)
	{
		throw Exception("Cannot retrieve DRM resources", errno);
	}
}

ModeResource::~ModeResource()
{
	DLOG("ModeResource", DEBUG) << "Delete";

	if (mData)
	{
		drmModeFreeResources(mData);
	}
}

/*******************************************************************************
 * ModeConnector
 ******************************************************************************/

ModeConnector::ModeConnector(int fd, int connectorId)
{
	DLOG("ModeConnector", DEBUG) << "Create, id: " << connectorId;

	mData = drmModeGetConnector(fd, connectorId);

	if (!mData)
	{
		throw Exception("Cannot retrieve DRM connector", errno);
	}
}

ModeConnector::~ModeConnector()
{
	if (mData)
	{
		DLOG("ModeConnector", DEBUG) << "Delete, id: "
									<< mData->connector_id;

		drmModeFreeConnector(mData);
	}
}

/*******************************************************************************
 * ModeEncoder
 ******************************************************************************/

ModeEncoder::ModeEncoder(int fd, int encoderId)
{
	mData = drmModeGetEncoder(fd, encoderId);

	if (!mData)
	{
		throw Exception("Cannot retrieve DRM encoder: " + to_string(encoderId),
						errno);
	}

	DLOG("ModeEncoder", DEBUG) << "Create ModeEncoder, id: " << encoderId
							  << ", crtc id: " << mData->crtc_id;
}

ModeEncoder::~ModeEncoder()
{
	if (mData)
	{
		DLOG("ModeEncoder", DEBUG) << "Delete ModeEncoder, id: "
								   << mData->encoder_id;

		drmModeFreeEncoder(mData);
	}
}

}
