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

#include "CommandHandler.hpp"

/***************************************************************************//**
 * @defgroup displ_be Display backend
 * Backend related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Ring buffer used for the connector control.
 * @ingroup displ_be
 ******************************************************************************/
class ConCtrlRingBuffer : public XenBackend::RingBufferInBase<
											xen_displif_back_ring,
											xen_displif_sring,
											xendispl_req,
											xendispl_resp>
{
public:
	/**
	 * @param display     display
	 * @param conId       connector id
	 * @param eventBuffer event ring buffer
	 * @param domId       frontend domain id
	 * @param port        event channel port number
	 * @param ref         grant table reference
	 */
	ConCtrlRingBuffer(std::shared_ptr<DisplayItf> display, uint32_t conId,
					  std::shared_ptr<ConEventRingBuffer> eventBuffer,
					  int domId, int port, int ref);

private:
	CommandHandler mCommandHandler;
	XenBackend::Log mLog;

	void processRequest(const xendispl_req& req);
};

/***************************************************************************//**
 * Display frontend handler.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param display   display
	 * @param conId     connector id
	 * @param domId     frontend domain id
	 * @param backend   backend instance
	 * @param id        frontend instance id
	 */
	DisplayFrontendHandler(std::shared_ptr<DisplayItf> display,
						   uint32_t conId, int domId,
						   XenBackend::BackendBase& backend, int id) :
		FrontendHandlerBase(domId, backend, id),
		mDisplay(display),
		mConId(conId),
		mLog("DisplayFrontend") {}

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind();

private:

	std::shared_ptr<DisplayItf> mDisplay;
	uint32_t mConId;
	XenBackend::Log mLog;

	void createConnector(const std::string& streamPath, int conId);
};

/***************************************************************************//**
 * Display backend class.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param domId         domain id
	 * @param deviceName    device name
	 * @param id            instance id
	 */
	DisplayBackend(int domId, const std::string& deviceName, int id);

protected:

	/**
	 * Is called when new display frontend appears.
	 * @param domId domain id
	 * @param id    instance id
	 */
	void onNewFrontend(int domId, int id);

private:

	std::shared_ptr<DisplayItf> mDisplay;

	uint32_t getConnectorId();
};

#endif /* DISPLAYBACKEND_HPP_ */
