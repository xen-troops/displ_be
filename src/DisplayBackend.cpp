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

#include <memory>
#include <vector>

#include <drm_fourcc.h>

#include <xen/be/XenStore.hpp>

#include "drm/Device.hpp"
#include "wayland/Display.hpp"

/***************************************************************************//**
 * @mainpage displ_be
 *
 * This backend implements virtual display devices. It is implemented based on
 * libxenbe.
 *
 ******************************************************************************/

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;

using XenBackend::FrontendHandlerBase;
using XenBackend::RingBufferBase;

const uint32_t cWlBackgroundWidth = 1920;
const uint32_t cWlBackgroundHeight = 1080;

/*******************************************************************************
 * ConCtrlRingBuffer
 ******************************************************************************/

ConCtrlRingBuffer::ConCtrlRingBuffer(shared_ptr<ConnectorItf> connector,
									 shared_ptr<BuffersStorage> buffersStorage,
									 shared_ptr<ConEventRingBuffer> eventBuffer,
									 domid_t domId,
									 evtchn_port_t port, grant_ref_t ref) :
	RingBufferInBase<xen_displif_back_ring, xen_displif_sring,
					 xendispl_req, xendispl_resp>(domId, port, ref),
	mCommandHandler(connector, buffersStorage, eventBuffer),
	mLog("ConCtrlRing")
{
	LOG(mLog, DEBUG) << "Create ctrl ring buffer";
}

void ConCtrlRingBuffer::processRequest(const xendispl_req& req)
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

	shared_ptr<ConEventRingBuffer> eventRingBuffer(
			new ConEventRingBuffer(conId, getDomId(), port, ref,
								   XENDISPL_IN_RING_OFFS,
								   XENDISPL_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);


	port = getXenStore().readInt(conPath + "/" +
								 XENDISPL_FIELD_CTRL_CHANNEL);

	ref = getXenStore().readInt(conPath + "/" +
								XENDISPL_FIELD_CTRL_RING_REF);

	if (mDisplayMode == DisplayMode::DRM)
	{
		conId = getDrmConnectorId();
	}
	else
	{
		uint32_t width, height;
		string res = getXenStore().readString(conPath + "/" +
											  XENDISPL_FIELD_RESOLUTION);

		convertResolution(res, width, height);

		conId = createWaylandConnector(width, height);
	}

	shared_ptr<RingBufferBase> ctrlRingBuffer(
			new ConCtrlRingBuffer(mDisplay->getConnectorById(conId),
								  mBuffersStorage,
								  eventRingBuffer,
								  getDomId(), port, ref));

	addRingBuffer(ctrlRingBuffer);
}

void DisplayFrontendHandler::convertResolution(const std::string& res,
											   uint32_t& width,
											   uint32_t& height)
{
	auto find = res.find_first_of(XENDISPL_RESOLUTION_SEPARATOR);

	if (find == string::npos)
	{
		throw DisplayItfException("Wrong format of resolution");
	}

	width = stoul(res.substr(0, find));
	height = stoul(res.substr(find + 1, string::npos));
}

uint32_t DisplayFrontendHandler::createWaylandConnector(uint32_t width,
														uint32_t height)
{
	auto wlDisplay = dynamic_pointer_cast<Wayland::Display>(mDisplay);

	wlDisplay->createConnector(mCurrentConId,
							   mCurrentConId * width, 0,
							   width, height);

	return mCurrentConId++;
}

uint32_t DisplayFrontendHandler::getDrmConnectorId()
{
	auto drmDevice = dynamic_pointer_cast<Drm::Device>(mDisplay);

	for (size_t i = 0; i < drmDevice->getConnectorsCount(); i++)
	{
		auto connector = drmDevice->getConnectorByIndex(i);

		if (connector->isConnected() && !connector->isInitialized())
		{
			return connector->getId();
		}
	}

	throw DisplayItfException("No available connectors found");
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

DisplayBackend::DisplayBackend(DisplayMode mode, domid_t domId,
							   const string& deviceName, int id) :
	BackendBase(domId, deviceName, id),
	mDisplayMode(mode)
{

	if (mDisplayMode == DisplayMode::DRM)
	{
		auto drmDevice = new Drm::Device("/dev/dri/card0");

		for (size_t i = 0; i < drmDevice->getConnectorsCount(); i++)
		{
			auto connector = drmDevice->getConnectorByIndex(i);

			LOG("Main", DEBUG) << "Connector: "
							   << connector->getId()
							   << ", connected: "
							   << connector->isConnected();
		}

		mDisplay.reset(drmDevice);
	}
	else
	{
		auto wlDisplay = new Wayland::Display();

		wlDisplay->createBackgroundSurface(cWlBackgroundWidth,
										   cWlBackgroundHeight);

		mDisplay.reset(wlDisplay);
	}

	mDisplay->start();
}

void DisplayBackend::onNewFrontend(domid_t domId, int id)
{

	addFrontendHandler(shared_ptr<FrontendHandlerBase>(
			new DisplayFrontendHandler(mDisplayMode, mDisplay,
									   domId, *this, id)));
}
