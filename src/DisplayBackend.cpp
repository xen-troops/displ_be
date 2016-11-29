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

#include <iostream>
#include <memory>
#include <vector>

#include <csignal>
#include <execinfo.h>
#include <getopt.h>

#include <xen/be/XenStore.hpp>

#include "drm/Device.hpp"

/***************************************************************************//**
 * @mainpage displ_be
 *
 * This backend implements virtual display devices. It is implemented based on
 * libxenbe.
 *
 ******************************************************************************/

using std::cout;
using std::dynamic_pointer_cast;
using std::endl;
using std::exception;
using std::shared_ptr;
using std::signal;
using std::stoi;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using XenBackend::FrontendHandlerBase;
using XenBackend::Log;
using XenBackend::RingBufferBase;
using XenBackend::RingBufferInBase;
using XenBackend::XenStore;

unique_ptr <DisplayBackend> gDisplayBackend;

/*******************************************************************************
 * ConCtrlRingBuffer
 ******************************************************************************/

ConCtrlRingBuffer::ConCtrlRingBuffer(shared_ptr<DisplayItf> display,
									 uint32_t conId,
									 shared_ptr<ConEventRingBuffer> eventBuffer,
									 int domId, int port, int ref) :
	RingBufferInBase<xen_displif_back_ring, xen_displif_sring,
					 xendispl_req, xendispl_resp>(domId, port, ref),
	mCommandHandler(display, conId, domId, eventBuffer),
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
	auto port = getXenStore().readInt(conPath + "/" +
									  XENDISPL_FIELD_EVT_CHANNEL);

	uint32_t ref = getXenStore().readInt(conPath + "/" +
										 XENDISPL_FIELD_EVT_RING_REF);

	shared_ptr<ConEventRingBuffer> eventRingBuffer(
			new ConEventRingBuffer(conId, getDomId(), port, ref,
								   XENDISPL_IN_RING_SIZE,
								   XENDISPL_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);


	port = getXenStore().readInt(conPath + "/" +
								 XENDISPL_FIELD_CTRL_CHANNEL);

	ref = getXenStore().readInt(conPath + "/" +
								XENDISPL_FIELD_CTRL_RING_REF);

	shared_ptr<RingBufferBase> ctrlRingBuffer(
			new ConCtrlRingBuffer(mDisplay, mConId, eventRingBuffer,
								  getDomId(), port, ref));

	addRingBuffer(ctrlRingBuffer);
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

DisplayBackend::DisplayBackend(int domId, const string& deviceName, int id) :
	BackendBase(domId, deviceName, id)
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

void DisplayBackend::onNewFrontend(int domId, int id)
{

	addFrontendHandler(shared_ptr<FrontendHandlerBase>(
			new DisplayFrontendHandler(mDisplay, getConnectorId(),
									   domId, *this, id)));
}

uint32_t DisplayBackend::getConnectorId()
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
 *
 ******************************************************************************/

void terminateHandler(int signal)
{
	gDisplayBackend->stop();
}

void segmentationHandler(int sig)
{
	void *array[20];
	size_t size;

	LOG("Main", ERROR) << "Segmentation fault!";

	size = backtrace(array, 2);

	backtrace_symbols_fd(array, size, STDERR_FILENO);

	exit(1);
}

void registerSignals()
{
	signal(SIGINT, terminateHandler);
	signal(SIGTERM, terminateHandler);
	signal(SIGSEGV, segmentationHandler);
}

bool commandLineOptions(int argc, char *argv[])
{

	int opt = -1;

	while((opt = getopt(argc, argv, "v:fh?")) != -1)
	{
		switch(opt)
		{
		case 'v':
			if (!Log::setLogLevel(string(optarg)))
			{
				return false;
			}

			break;

		case 'f':
			Log::setShowFileAndLine(true);
			break;

		default:
			return false;
		}
	}

	return true;
}

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{
			gDisplayBackend.reset(new DisplayBackend(0, XENDISPL_DRIVER_NAME, 0));

			gDisplayBackend->run();
		}
		else
		{
			cout << "Usage: " << argv[0] << " [-v <level>]" << endl;
			cout << "\t-v -- verbose level "
				 << "(disable, error, warning, info, debug)" << endl;
		}
	}
	catch(const exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}
	catch(...)
	{
		LOG("Main", ERROR) << "Unknown error";
	}

	return 0;
}
