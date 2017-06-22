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


template <typename T>
/***************************************************************************//**
 * Mode data template.
 * It provides pointer like functionality for mode data.
 * @ingroup drm
 ******************************************************************************/
class ModeData
{
public:

	ModeData() {};
	ModeData(const ModeData&) = delete;
	ModeData& operator=(ModeData const&) = delete;

	/**
	 * Provide access to the DRM mode data fields.
	 */
	const T operator->() const { return mData; }

	/**
	 * Provide access to the DRM mode data structure.
	 */
	const T& operator*() const { return *mData; }

protected:

	/**
	 * Mode data object
	 */
	T mData;
};

/***************************************************************************//**
 * Wrapper for DRM mode resource object.
 * It creates the DRM mode resource object in the constructor and
 * deletes it in the destructor.
 * @ingroup drm
 ******************************************************************************/
class ModeResource : public ModeData<drmModeResPtr>
{
public:

	/**
	 * @param fd DRM device file descriptor
	 */
	explicit ModeResource(int fd);

	~ModeResource();
};

/***************************************************************************//**
 * Wrapper for DRM mode connector object.
 * It creates the DRM mode connector object in the constructor and
 * deletes it in the destructor.
 * @ingroup drm
 ******************************************************************************/
class ModeConnector : public ModeData<drmModeConnectorPtr>
{
public:

	/**
	 * @param fd          DRM device file descriptor
	 * @param connectorId connector id
	 */
	ModeConnector(int fd, int connectorId);

	~ModeConnector();
};

/***************************************************************************//**
 * Wrapper for DRM mode encoder object.
 * It creates the DRM mode connector object in the constructor and
 * deletes it in the destructor.
 * @ingroup drm
 ******************************************************************************/
class ModeEncoder : public ModeData<drmModeEncoderPtr>
{
public:

	/**
	 * @param fd        DRM device file descriptor
	 * @param encoderId encoder id
	 */
	ModeEncoder(int fd, int encoderId);

	~ModeEncoder();
};

}

#endif /* SRC_DRM_MODES_HPP_ */
