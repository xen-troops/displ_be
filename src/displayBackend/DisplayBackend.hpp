/*
 *  Display backend
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
 */

#ifndef DISPLAYBACKEND_HPP_
#define DISPLAYBACKEND_HPP_

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

#include "DisplayCommandHandler.hpp"

/***************************************************************************//**
 * @defgroup displ_be Display backend
 * Backend related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Ring buffer used for the connector control.
 * @ingroup displ_be
 ******************************************************************************/
class CtrlRingBuffer : public XenBackend::RingBufferInBase<
		xen_displif_back_ring, xen_displif_sring, xendispl_req, xendispl_resp>
{
public:
	/**
	 * @param display        display object
	 * @param connector      connector object
	 * @param buffersStorage buffers storage
	 * @param eventBuffer    event ring buffer
	 * @param domId          frontend domain id
	 * @param port           event channel port number
	 * @param ref            grant table reference
	 */
	CtrlRingBuffer(DisplayItf::DisplayPtr display,
				   DisplayItf::ConnectorPtr connector,
				   BuffersStoragePtr buffersStorage,
				   EventRingBufferPtr eventBuffer,
				   domid_t domId, evtchn_port_t port, grant_ref_t ref);

private:

	DisplayCommandHandler mCommandHandler;
	XenBackend::Log mLog;

	void processRequest(const xendispl_req& req);
};

typedef std::shared_ptr<CtrlRingBuffer> CtrlRingBufferPtr;

/***************************************************************************//**
 * Display frontend handler.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param display   display
	 * @param devName   device name
	 * @param domId     frontend domain id
	 * @param devId     frontend device id
	 */
	DisplayFrontendHandler(DisplayItf::DisplayPtr display,
						   const std::string& devName,
						   domid_t domId, uint16_t devId) :
		FrontendHandlerBase("DisplFrontend", devName, domId, devId),
		mDisplay(display),
		mLog("DisplFrontend") {}

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind() override;

private:

	DisplayItf::DisplayPtr mDisplay;
	XenBackend::Log mLog;

	void createConnector(const std::string& streamPath, int conIndex,
						 BuffersStoragePtr bufferStorage);
};

/***************************************************************************//**
 * Display backend class.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param display       display
	 * @param deviceName    device name
	 * @param domId         domain id
	 * @param devId         device id
	 */
	DisplayBackend(DisplayItf::DisplayPtr display,
				   const std::string& deviceName);

protected:

	/**
	 * Is called when new display frontend appears.
	 * @param domId domain id
	 * @param devId device id
	 */
	void onNewFrontend(domid_t domId, uint16_t devId);

private:

	DisplayItf::DisplayPtr mDisplay;
};

#endif /* DISPLAYBACKEND_HPP_ */
