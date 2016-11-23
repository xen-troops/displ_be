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

#ifndef SRC_DRM_MODES_HPP_
#define SRC_DRM_MODES_HPP_

#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Drm {

class ModeResource
{
public:
	explicit ModeResource(int fd);
	~ModeResource();
	ModeResource(const ModeResource&) = delete;
	ModeResource& operator=(ModeResource const&) = delete;

	const drmModeResPtr operator->() const;
	const drmModeRes& operator*() const;

private:
	drmModeResPtr mRes;
};

class ModeConnector
{
public:
	ModeConnector(int fd, int connectorId);
	~ModeConnector();
	ModeConnector(const ModeConnector&) = delete;
	ModeConnector& operator=(ModeConnector const&) = delete;

	const drmModeConnectorPtr operator->() const;
	const drmModeConnector& operator*() const;

private:
	drmModeConnectorPtr mConnector;
};

class ModeEncoder
{
public:
	ModeEncoder(int fd, int encoderId);
	~ModeEncoder();
	ModeEncoder(const ModeEncoder&) = delete;
	ModeEncoder& operator=(ModeEncoder const&) = delete;

	const drmModeEncoderPtr operator->() const;
	const drmModeEncoder& operator*() const;

private:
	drmModeEncoderPtr mEncoder;
};

}

#endif /* SRC_DRM_MODES_HPP_ */
