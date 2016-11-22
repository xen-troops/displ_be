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

#include "Device.hpp"

using std::to_string;

namespace Drm {

/***************************************************************************//**
 * Connector
 ******************************************************************************/

Connector::Connector(Device& dev, int connectorId) :
	mDev(dev),
	mFd(dev.getFd()),
	mCrtcId(cInvalidId),
	mConnector(mFd, connectorId),
	mSavedCrtc(nullptr),
	mLog("Connector(" + to_string(connectorId) + ")")
{
	LOG(mLog, DEBUG) << "Create connector";
}

Connector::~Connector()
{
	release();

	LOG(mLog, DEBUG) << "Delete connector";
}

/***************************************************************************//**
 * Public
 ******************************************************************************/

void Connector::init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
					 uint32_t bpp, uint32_t fbId)
{

	if (mConnector->connection != DRM_MODE_CONNECTED)
	{
		throw DrmException("Connector is not connected");
	}

	if (mCrtcId != cInvalidId)
	{
		throw DrmException("Already initialized");
	}

	LOG(mLog, DEBUG) << "Init, w: " << width << ", h: " << height
					 << ", bpp " << bpp << ", fb id: " << fbId;

	mCrtcId = findCrtcId();

	if (mCrtcId == cInvalidId)
	{
		throw DrmException("Cannot find CRTC for connector");
	}

	auto mode = findMode(width, height);

	if (!mode)
	{
		throw DrmException("Unsupported mode");
	}

	mSavedCrtc = drmModeGetCrtc(mFd, mCrtcId);

	if (drmModeSetCrtc(mFd, mCrtcId, fbId, 0, 0,
					   &mConnector->connector_id, 1, mode))
	{
		throw DrmException("Cannot set CRTC for connector");
	}
}

void Connector::release()
{
	mCrtcId = cInvalidId;

	if (mSavedCrtc)
	{
		LOG(mLog, DEBUG) << "Release";

		drmModeSetCrtc(mFd, mSavedCrtc->crtc_id, mSavedCrtc->buffer_id,
					   mSavedCrtc->x, mSavedCrtc->y, &mConnector->connector_id,
					   1, &mSavedCrtc->mode);

		drmModeFreeCrtc(mSavedCrtc);

		mSavedCrtc = nullptr;
	}
}

/***************************************************************************//**
 * Private
 ******************************************************************************/

uint32_t Connector::findCrtcId()
{
	auto crtcId = getAssignedCrtcId();

	if (crtcId != cInvalidId)
	{
		return crtcId;
	}

	return findMatchingCrtcId();
}

bool Connector::isCrtcIdUsedByOther(uint32_t crtcId)
{
	for (size_t i = 0; i < mDev.getConnectorsCount(); i++)
	{
		if (crtcId == mDev.getConnectorByIndex(i).getCrtcId())
		{
			LOG(mLog, DEBUG) << "Crtc id is used by other connector";

			return true;
		}
	}

	return false;
}

uint32_t Connector::getAssignedCrtcId()
{
	auto encoderId = mConnector->encoder_id;

	if (encoderId != cInvalidId)
	{
		ModeEncoder encoder(mFd, encoderId);

		auto crtcId = encoder->crtc_id;

		if (crtcId != cInvalidId)
		{

			if (isCrtcIdUsedByOther(crtcId))
			{
				return cInvalidId;
			}

			LOG(mLog, DEBUG) << "Assigned crtc id: " << crtcId;

			return crtcId;
		}
	}

	return cInvalidId;
}

uint32_t Connector::findMatchingCrtcId()
{
	for (int encIndex = 0; encIndex < mConnector->count_encoders; encIndex++)
	{
		ModeEncoder encoder(mFd, mConnector->encoders[encIndex]);

		for (int crtcIndex = 0; crtcIndex < mDev.getCtrcsCount(); crtcIndex++)
		{
			if (!(encoder->possible_crtcs & (1 << crtcIndex)))
			{
				continue;
			}

			auto crtcId = mDev.getCtrcIdByIndex(crtcIndex);

			if (isCrtcIdUsedByOther(crtcId))
			{
				continue;
			}

			LOG(mLog, DEBUG) << "Matched crtc found: " << crtcId;

			return crtcId;
		}
	}

	return cInvalidId;
}

drmModeModeInfoPtr Connector::findMode(uint32_t width, uint32_t height)
{
	for (int i = 0; i < mConnector->count_modes; i++)
	{
		if (mConnector->modes[i].hdisplay == width &&
			mConnector->modes[i].vdisplay == height)
		{
			LOG(mLog, DEBUG) << "Found mode: " << mConnector->modes[i].name;

			return &mConnector->modes[i];
		}
	}

	return nullptr;
}

}
