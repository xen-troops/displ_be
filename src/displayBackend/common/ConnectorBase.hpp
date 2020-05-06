/*
 *  Base connector class
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
 * Copyright (C) 2020 EPAM Systems Inc.
 *
 */

#ifndef SRC_CONNECTOR_BASE_HPP_
#define SRC_CONNECTOR_BASE_HPP_

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"

/***************************************************************************//**
 * Base virtual connector class.
 * @ingroup displ_be
 ******************************************************************************/
class ConnectorBase : public DisplayItf::Connector
{
protected:

	domid_t mDomId;

	/* Width and height read from XenStore's connector resolution field. */
	uint32_t mCfgWidth;
	uint32_t mCfgHeight;

	XenBackend::Log mLog;

	/*
	 * @param domId  domain ID
	 * @param width  connector width as configured in XenStore
	 * @param height connector height as configured in XenStore
	 */
	ConnectorBase(domid_t domId, uint32_t width, uint32_t height);
};

#endif /* SRC_CONNECTOR_BASE_HPP_ */
