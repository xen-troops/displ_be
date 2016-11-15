/*
 *  DRM backend
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

#include "DrmBackend.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include <csignal>

#include <getopt.h>

#include <xen/be/XenStore.hpp>
#include "Drm.hpp"

/***************************************************************************//**
 * @mainpage drm_be
 *
 * This backend implements virtual DRM devices. It is implemented with
 * libxenbe.
 *
 ******************************************************************************/

using std::cout;
using std::endl;
using std::exception;
using std::shared_ptr;
using std::signal;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using XenBackend::FrontendHandlerBase;
using XenBackend::Log;
using XenBackend::RingBufferBase;
using XenBackend::RingBufferInBase;
using XenBackend::XenStore;

using Drm::DrmDevice;

unique_ptr <DrmBackend> gDrmBackend;

#if 0

/local/domain/19/device/vdrm = ""
/local/domain/19/device/vdrm/0 = ""
/local/domain/19/device/vdrm/0/backend-id = "0"
/local/domain/19/device/vdrm/0/backend = "/local/domain/0/backend/vdrm/19/0"
/local/domain/19/device/vdrm/0/state = "4"
/local/domain/19/device/vdrm/0/connector = ""
/local/domain/19/device/vdrm/0/connector/0 = ""
/local/domain/19/device/vdrm/0/connector/0/type = "DP"
/local/domain/19/device/vdrm/0/connector/0/id = "1"
/local/domain/19/device/vdrm/0/connector/0/resolution = "1200x800"
/local/domain/19/device/vdrm/0/connector/0/ctrl-ring-ref = "1684"
/local/domain/19/device/vdrm/0/connector/0/ctrl-channel = "22"
/local/domain/19/device/vdrm/0/connector/0/event-ring-ref = "1685"
/local/domain/19/device/vdrm/0/connector/0/event-channel = "23"
/local/domain/19/device/vdrm/0/connector/1 = ""
/local/domain/19/device/vdrm/0/connector/1/type = "HDMI-A"
/local/domain/19/device/vdrm/0/connector/1/id = "8"
/local/domain/19/device/vdrm/0/connector/1/resolution = "1920x1200"
/local/domain/19/device/vdrm/0/connector/1/ctrl-ring-ref = "1686"
/local/domain/19/device/vdrm/0/connector/1/ctrl-channel = "24"
/local/domain/19/device/vdrm/0/connector/1/event-ring-ref = "1687"
/local/domain/19/device/vdrm/0/connector/1/event-channel = "25"
/local/domain/19/control = ""

#endif

/***************************************************************************//**
 * ConCtrlRingBuffer
 ******************************************************************************/

ConCtrlRingBuffer::ConCtrlRingBuffer(DrmDevice& drm,
									 shared_ptr<ConEventRingBuffer> eventBuffer,
									 int id, int domId, int port, int ref) :
	RingBufferInBase<xen_drmif_back_ring, xen_drmif_sring,
					 xendrm_req, xendrm_resp>(domId, port, ref),
	mId(id),
	mCommandHandler(mId, domId, drm, eventBuffer),
	mLog("ConCtrlRing(" + to_string(id) + ")")
{
	LOG(mLog, DEBUG) << "Create connector control ring buffer: id = " << mId;
}

void ConCtrlRingBuffer::processRequest(const xendrm_req& req)
{
	DLOG(mLog, DEBUG) << "Request received, id: " << mId
			<< ", cmd:" << static_cast<int>(req.u.data.operation);

	xendrm_resp rsp {};

	rsp.u.data.id = req.u.data.id;
	rsp.u.data.id = req.u.data.id;
	rsp.u.data.operation = req.u.data.operation;
	rsp.u.data.status = mCommandHandler.processCommand(req);

	sendResponse(rsp);
}

/***************************************************************************//**
 * ConEventRingBuffer
 ******************************************************************************/

ConEventRingBuffer::ConEventRingBuffer(int id, int domId, int port,
									   int ref, int offset, int numEvents) :
	RingBufferOutBase<xendrm_event_page, xendrm_evt>(domId, port, ref,
													 offset, numEvents),
	mId(id),
	mLog("ConEventRing(" + to_string(id) + ")")
{
	LOG(mLog, DEBUG) << "Create connector event ring buffer: id = " << mId;
}

/***************************************************************************//**
 * DrmFrontendHandler
 ******************************************************************************/

void DrmFrontendHandler::onBind()
{
	string conBasePath = getXsFrontendPath() + "/" + XENDRM_PATH_CONNECTOR;

	const vector<string> cons = getXenStore().readDirectory(conBasePath);

	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	if (cons.size() == 0)
	{
		LOG(mLog, WARNING) << "No DRM connectors found : " << getDomId();
	}

	for(auto conId : cons)
	{
		LOG(mLog, DEBUG) << "Found connector: " << conId;

		createConnector(conBasePath + "/" + conId);
	}

	mDrm.start();
}

void DrmFrontendHandler::createConnector(const string& conPath)
{
	auto id = getXenStore().readInt(conPath + "/" + XENDRM_FIELD_ID);

	auto port = getXenStore().readInt(conPath + "/" + XENDRM_FIELD_EVT_CHANNEL);

	uint32_t ref = getXenStore().readInt(conPath + "/" +
										 XENDRM_FIELD_EVT_RING_REF);

	shared_ptr<ConEventRingBuffer> eventRingBuffer(
			new ConEventRingBuffer(id, getDomId(), port, ref,
								XENDRM_IN_RING_SIZE, XENDRM_IN_RING_LEN));

	addRingBuffer(eventRingBuffer);


	port = getXenStore().readInt(conPath + "/" +
									  XENDRM_FIELD_CTRL_CHANNEL);

	ref = getXenStore().readInt(conPath + "/" +
										 XENDRM_FIELD_CTRL_RING_REF);

	shared_ptr<RingBufferBase> ctrlRingBuffer(
			new ConCtrlRingBuffer(mDrm, eventRingBuffer, id,
								  getDomId(), port, ref));

	addRingBuffer(ctrlRingBuffer);

}

/***************************************************************************//**
 * DrmBackend
 ******************************************************************************/

void DrmBackend::onNewFrontend(int domId, int id)
{
	addFrontendHandler(shared_ptr<FrontendHandlerBase>(
					   new DrmFrontendHandler("dev/dri/card0", domId,
							   *this, id)));
}

/***************************************************************************//**
 *
 ******************************************************************************/

void terminateHandler(int signal)
{
	gDrmBackend->stop();
}

void segmentationHandler(int sig)
{
	LOG("Main", ERROR) << "Unknown error!";
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

uint8_t nextColor(bool *up, uint8_t cur, unsigned int mod)
{
	uint8_t next;

	next = cur + (*up ? 1 : -1) * (rand() % mod);
	if ((*up && next < cur) || (!*up && next > cur)) {
		*up = !*up;
		next = cur;
	}

	return next;
}

void draw(Drm::FrameBuffer& fb1, Drm::FrameBuffer& fb2, uint32_t crtcId)
{
	static uint8_t r, g, b {0};
	static bool r_up, g_up, b_up {true};

	unsigned int j, k, off;

	r = nextColor(&r_up, r, 20);
	g = nextColor(&g_up, g, 10);
	b = nextColor(&b_up, b, 5);

//	LOG("Main", DEBUG) << "Flip: " << fb1.getId();

	const Drm::Dumb& dumb = fb1.getDumb();

	for (j = 0; j < dumb.getHeight(); ++j)
	{
		for (k = 0; k < dumb.getWidth(); ++k)
		{
			off = dumb.getPitch() * j + k * 4;

			uint8_t* buffer = static_cast<uint8_t*>(dumb.getBuffer());
			*(uint32_t*)&buffer[off] = (r << 16) | (g << 8) | b;
		}
	}

	fb1.pageFlip(crtcId, std::bind(draw, std::ref(fb2), std::ref(fb1), crtcId));
}

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{
			DrmDevice drm("/dev/dri/card0");
/*
			drm.createDumb(1920, 1080, 32);
			drm.createFrameBuffer(drm.createDumb(800, 600, 32), 800, 600,
								  DRM_FORMAT_XRGB8888);

*/
/*
			for (size_t i = 0; i < drm.getConnectorsCount(); i++)
			{
				Drm::Connector& connector = drm.getConnectorByIndex(i);
				LOG("Main", DEBUG) << "Connector: "
								   << connector.getId()
								   << ", connected: "
								   << connector.isConnected();
			}
*/

			Drm::Dumb& dumb1 = drm.createDumb(1920, 1080, 32);
			Drm::FrameBuffer& fb1 = drm.createFrameBuffer(dumb1, 1920,
														  1080, DRM_FORMAT_XRGB8888);

			Drm::Dumb& dumb2 = drm.createDumb(1920, 1080, 32);
			Drm::FrameBuffer& fb2 = drm.createFrameBuffer(dumb2, 1920, 1080,
														  DRM_FORMAT_XRGB8888);

			Drm::Connector& connector = drm.getConnectorById(46);

			connector.init(0, 0, 1920, 1080, 32, fb1.getId());

			drm.start();

			draw(const_cast<Drm::FrameBuffer&>(fb1), const_cast<Drm::FrameBuffer&>(fb2), connector.getCrtcId());

			std::this_thread::sleep_for(std::chrono::seconds(6));


			#if 0
			gDrmBackend.reset(new DrmBackend(0, XENDRM_DRIVER_NAME));

			gDrmBackend->run();
#endif
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
