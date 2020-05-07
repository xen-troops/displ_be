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
	XenBackend::Log mLog;

	ConnectorBase(domid_t domId);
};

#endif /* SRC_CONNECTOR_BASE_HPP_ */
