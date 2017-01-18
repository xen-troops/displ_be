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

#include "DisplayBackend.hpp"

#include <vector>

#include <drm_fourcc.h>

#include <xen/be/XenStore.hpp>

#include "drm/Display.hpp"
#include "wayland/Display.hpp"

/***************************************************************************//**
 * @mainpage displ_be
 *
 * This backend implements virtual display devices. It is implemented based on
 * libxenbe.
 *
 ******************************************************************************/

using std::string;
using std::vector;

using XenBackend::FrontendHandlerPtr;

using DisplayItf::ConnectorPtr;
using DisplayItf::DisplayPtr;

/*******************************************************************************
 * ConCtrlRingBuffer
 ******************************************************************************/

CtrlRingBuffer::CtrlRingBuffer(ConnectorPtr connector,
							   BuffersStoragePtr buffersStorage,
							   EventRingBufferPtr eventBuffer,
							   domid_t domId,
							   evtchn_port_t port, grant_ref_t ref) :
	RingBufferInBase<xen_displif_back_ring, xen_displif_sring,
					 xendispl_req, xendispl_resp>(domId, port, ref),
	mCommandHandler(connector, buffersStorage, eventBuffer),
	mLog("ConCtrlRing")
{
	LOG(mLog, DEBUG) << "Create ctrl ring buffer";
}

void CtrlRingBuffer::processRequest(const xendispl_req& req)
{
	DLOG(mLog, DEBUG) << "Request received, cmd:"
					  << static_cast<int>(req.operation);

	xendispl_resp rsp {};

	rsp.id = req.id;
	rsp.id = req.id;
	rsp.operation = req.operation;
	rsp.status = mCommandHandler.processCommand(req);

	sendResponse(rsp);
}

/*******************************************************************************
 * DisplayFrontendHandler
 ******************************************************************************/

void DisplayFrontendHandler::onBind()
{
	string conBasePath = getXsFrontendPath() + "/" + XENDISPL_PATH_CONNECTOR;

	const vector<string> cons = getXenStore().readDirectory(conBasePath);

	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	if (cons.size() == 0)
	{
		LOG(mLog, WARNING) << "No display connectors found : " << getDomId();
	}

	for(auto conId : cons)
	{
		LOG(mLog, DEBUG) << "Found connector: " << conId;

		createConnector(conBasePath + "/" + conId, stoi(conId));
	}
}

void DisplayFrontendHandler::createConnector(const string& conPath, int conId)
{
	evtchn_port_t port = getXenStore().readInt(conPath + "/" +
											   XENDISPL_FIELD_EVT_CHANNEL);

	uint32_t ref = getXenStore().readInt(conPath + "/" +
										 XENDISPL_FIELD_EVT_RING_REF);

	EventRingBufferPtr eventRingBuffer(new EventRingBuffer(conId, getDomId(),
			port, ref, XENDISPL_IN_RING_OFFS, XENDISPL_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);


	port = getXenStore().readInt(conPath + "/" +
								 XENDISPL_FIELD_CTRL_CHANNEL);

	ref = getXenStore().readInt(conPath + "/" +
								XENDISPL_FIELD_CTRL_RING_REF);

	CtrlRingBufferPtr ctrlRingBuffer(
			new CtrlRingBuffer(mDisplay->getConnectorById(conId),
							   mBuffersStorage,
							   eventRingBuffer,
							   getDomId(), port, ref));

	addRingBuffer(ctrlRingBuffer);
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

DisplayBackend::DisplayBackend(DisplayPtr display,
							   const string& deviceName,
							   domid_t domId, int id) :
	BackendBase("DisplBackend", deviceName, domId, id),
	mDisplay(display)
{
	mDisplay->start();
}

void DisplayBackend::onNewFrontend(domid_t domId, int id)
{
	addFrontendHandler(FrontendHandlerPtr(
			new DisplayFrontendHandler(mDisplay, *this, domId, id)));
}
