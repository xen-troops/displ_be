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

#include "Exception.hpp"

#include <xen/be/Log.hpp>

using std::to_string;

namespace Drm {

/*******************************************************************************
 * ModeResource
 ******************************************************************************/

ModeResource::ModeResource(int fd)
{
	DLOG("ModeResource", DEBUG) << "Create";

	mRes = drmModeGetResources(fd);

	if (!mRes)
	{
		throw DrmException("Cannot retrieve DRM resources");
	}
}

ModeResource::~ModeResource()
{
	DLOG("ModeResource", DEBUG) << "Delete";

	if (mRes)
	{
		drmModeFreeResources(mRes);
	}
}

const drmModeResPtr ModeResource::operator->() const
{
	return mRes;
}

const drmModeRes& ModeResource::operator*() const
{
	return *mRes;
}

/*******************************************************************************
 * ModeConnector
 ******************************************************************************/

ModeConnector::ModeConnector(int fd, int connectorId)
{
	DLOG("ModeConnector", DEBUG) << "Create, id: " << connectorId;

	mConnector = drmModeGetConnector(fd, connectorId);

	if (!mConnector)
	{
		throw DrmException("Cannot retrieve DRM connector");
	}
}

ModeConnector::~ModeConnector()
{
	if (mConnector)
	{
		DLOG("ModeConnector", DEBUG) << "Delete, id: "
									<< mConnector->connector_id;

		drmModeFreeConnector(mConnector);
	}
}

const drmModeConnectorPtr ModeConnector::operator->() const
{
	return mConnector;
}

const drmModeConnector& ModeConnector::operator*() const
{
	return *mConnector;
}

/*******************************************************************************
 * ModeEncoder
 ******************************************************************************/

ModeEncoder::ModeEncoder(int fd, int encoderId)
{
	mEncoder = drmModeGetEncoder(fd, encoderId);

	if (!mEncoder)
	{
		throw DrmException("Cannot retrieve DRM encoder: " + to_string(encoderId));
	}

	DLOG("ModeEncoder", DEBUG) << "Create ModeEncoder, id: " << encoderId
							  << ", crtc id: " << mEncoder->crtc_id;
}

ModeEncoder::~ModeEncoder()
{
	if (mEncoder)
	{
		DLOG("ModeEncoder", DEBUG) << "Delete ModeEncoder, id: "
								   << mEncoder->encoder_id;

		drmModeFreeEncoder(mEncoder);
	}
}

const drmModeEncoderPtr ModeEncoder::operator->() const
{
	return mEncoder;
}

const drmModeEncoder& ModeEncoder::operator*() const
{
	return *mEncoder;
}

}
