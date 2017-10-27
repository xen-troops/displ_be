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

#include <algorithm>
#include <vector>

#include <xen/be/XenStore.hpp>

#ifdef WITH_IVI_EXTENSION
#include "wayland/Connector.hpp"
#endif

/***************************************************************************//**
 * @mainpage displ_be
 *
 * This backend implements virtual display devices. It is implemented based on
 * libxenbe.
 *
 ******************************************************************************/

using std::begin;
using std::dynamic_pointer_cast;
using std::end;
using std::find_if;
using std::string;
using std::to_string;
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
	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	BuffersStoragePtr buffersStorage(new BuffersStorage(getDomId(), mDisplay));

	string conBasePath = getXsFrontendPath() + "/";
	int conIndex = 0;

	while(getXenStore().checkIfExist(conBasePath + to_string(conIndex)))
	{
		LOG(mLog, DEBUG) << "Found connector: " << conIndex;

		createConnector(conBasePath + to_string(conIndex) + "/",
						conIndex, buffersStorage);

		conIndex++;
	}
}

void DisplayFrontendHandler::onClosing()
{
	LOG(mLog, DEBUG) << "On frontend closing : " << getDomId();
}

void DisplayFrontendHandler::createConnector(const string& conPath,
											 int conIndex,
											 BuffersStoragePtr bufferStorage)
{
	evtchn_port_t port = getXenStore().readInt(conPath +
											   XENDISPL_FIELD_EVT_CHANNEL);

	uint32_t ref = getXenStore().readInt(conPath +
										 XENDISPL_FIELD_EVT_RING_REF);

	EventRingBufferPtr eventRingBuffer(new EventRingBuffer(conIndex, getDomId(),
			port, ref, XENDISPL_IN_RING_OFFS, XENDISPL_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);

	port = getXenStore().readInt(conPath + XENDISPL_FIELD_REQ_CHANNEL);

	ref = getXenStore().readInt(conPath + XENDISPL_FIELD_REQ_RING_REF);

	vector<Config::Connector> connectors;

	mConfig->getConnectors(connectors);

	auto id = getXenStore().readString(conPath + "id");

	auto it = find_if(begin(connectors), end(connectors),
					  [&id](const Config::Connector& connector)
					  { return connector.id == id; });

	if (it == end(connectors))
	{
		LOG(mLog, WARNING) << "Connector: " << id << " is not configured";

		return;
	}

	auto connector = mDisplay->getConnectorByName(it->name);

	CtrlRingBufferPtr ctrlRingBuffer(
			new CtrlRingBuffer(connector,
							   bufferStorage,
							   eventRingBuffer,
							   getDomId(), port, ref));

	addRingBuffer(ctrlRingBuffer);
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

DisplayBackend::DisplayBackend(ConfigPtr config,
							   DisplayPtr display,
							   const string& deviceName) :
	BackendBase("DisplBackend", deviceName),
	mConfig(config),
	mDisplay(display)
{
	mDisplay->start();
}

void DisplayBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
	addFrontendHandler(FrontendHandlerPtr(
			new DisplayFrontendHandler(mConfig, mDisplay, getDeviceName(),
									   getDomId(), domId, devId)));
}
