/*
 *  Command handler
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

#include <iomanip>

#include <cstddef>
#include <cstring>
#include <sys/mman.h>

#include <wayland-client.h>
#include <drm_fourcc.h>
#include "DisplayCommandHandler.hpp"

using std::hex;
using std::exception;
using std::out_of_range;
using std::setfill;
using std::setw;
using std::shared_ptr;
using std::unordered_map;

using XenBackend::XenException;
using XenBackend::XenGnttabBuffer;

unordered_map<int, DisplayCommandHandler::CommandFn>
	DisplayCommandHandler::sCmdTable =
{
	{XENDISPL_OP_PG_FLIP,		&DisplayCommandHandler::pageFlip},
	{XENDISPL_OP_FB_ATTACH,		&DisplayCommandHandler::attachFrameBuffer},
	{XENDISPL_OP_FB_DETACH,		&DisplayCommandHandler::detachFrameBuffer},
	{XENDISPL_OP_DBUF_CREATE,	&DisplayCommandHandler::createDisplayBuffer},
	{XENDISPL_OP_DBUF_DESTROY,	&DisplayCommandHandler::destroyDisplayBuffer},
	{XENDISPL_OP_SET_CONFIG,	&DisplayCommandHandler::setConfig}
};

/*******************************************************************************
 * ConEventRingBuffer
 ******************************************************************************/

ConEventRingBuffer::ConEventRingBuffer(int id, domid_t domId,
									   evtchn_port_t port, grant_ref_t ref,
									   int offset, size_t size) :
	RingBufferOutBase<xendispl_event_page, xendispl_evt>(domId, port, ref,
														 offset, size),
	mId(id),
	mLog("ConEventRing")
{
	LOG(mLog, DEBUG) << "Create event ring buffer: id = " << mId;
}

/*******************************************************************************
 * CommandHandler
 ******************************************************************************/

DisplayCommandHandler::DisplayCommandHandler(shared_ptr<ConnectorItf> connector,
							   shared_ptr<BuffersStorage> buffersStorage,
							   shared_ptr<ConEventRingBuffer> eventBuffer) :
	mConnector(connector),
	mBuffersStorage(buffersStorage),
	mEventBuffer(eventBuffer),
	mEventId(0),
	mLog("CommandHandler")
{
	LOG(mLog, DEBUG) << "Create command handler, con ID: "
					 << mConnector->getId();
}

DisplayCommandHandler::~DisplayCommandHandler()
{
	LOG(mLog, DEBUG) << "Delete command handler, con ID: "
					 << mConnector->getId();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint8_t DisplayCommandHandler::processCommand(const xendispl_req& req)
{
	uint8_t status = XENDISPL_RSP_OKAY;

	try
	{
		(this->*sCmdTable.at(req.operation))(req);
	}
	catch(const out_of_range& e)
	{
		LOG(mLog, ERROR) << e.what();

		status = XENDISPL_RSP_NOTSUPP;
	}
	catch(const exception& e)
	{
		LOG(mLog, ERROR) << e.what();

		status = XENDISPL_RSP_ERROR;
	}

	DLOG(mLog, DEBUG) << "Return status: ["
					  << static_cast<signed int>(status) << "]";

	return status;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void DisplayCommandHandler::pageFlip(const xendispl_req& req)
{
	xendispl_page_flip_req flipReq = req.op.pg_flip;

	auto cookie = flipReq.fb_cookie;

	DLOG(mLog, DEBUG) << "Handle command [PAGE FLIP], fb ID: "
					  << hex << setfill('0') << setw(16)
					  << cookie;

	mBuffersStorage->copyBuffer(cookie);

	mConnector->pageFlip(mBuffersStorage->getFrameBuffer(flipReq.fb_cookie),
						 [cookie, this] () { sendFlipEvent(cookie); });
}

void DisplayCommandHandler::createDisplayBuffer(const xendispl_req& req)
{
	const xendispl_dbuf_create_req* dbufReq = &req.op.dbuf_create;

	DLOG(mLog, DEBUG) << "Handle command [CREATE DBUF], cookie: "
					  << hex << setfill('0') << setw(16)
					  << dbufReq->dbuf_cookie;

	mBuffersStorage->createDisplayBuffer(dbufReq->dbuf_cookie,
										 dbufReq->gref_directory_start,
										 dbufReq->buffer_sz,
										 dbufReq->width, dbufReq->height,
										 dbufReq->bpp);
}

void DisplayCommandHandler::destroyDisplayBuffer(const xendispl_req& req)
{
	const xendispl_dbuf_destroy_req* dbufReq = &req.op.dbuf_destroy;

	DLOG(mLog, DEBUG) << "Handle command [DESTROY DBUF], cookie: "
					  << hex << setfill('0') << setw(16)
					  << dbufReq->dbuf_cookie;

	mBuffersStorage->destroyDisplayBuffer(dbufReq->dbuf_cookie);
}

void DisplayCommandHandler::attachFrameBuffer(const xendispl_req& req)
{
	const xendispl_fb_attach_req* fbReq = &req.op.fb_attach;

	DLOG(mLog, DEBUG) << "Handle command [ATTACH FB], DB cookie: "
					  << hex << setfill('0') << setw(16)
					  << fbReq->dbuf_cookie << ", FB cookie: "
					  << setw(16) << fbReq->fb_cookie;

	mBuffersStorage->createFrameBuffer(fbReq->dbuf_cookie, fbReq->fb_cookie,
									   fbReq->width, fbReq->height,
									   fbReq->pixel_format);

}

void DisplayCommandHandler::detachFrameBuffer(const xendispl_req& req)
{
	const xendispl_fb_detach_req* fbReq = &req.op.fb_detach;

	DLOG(mLog, DEBUG) << "Handle command [DETACH FB], cookie: "
					  << hex << setfill('0') << setw(16)
					  << fbReq->fb_cookie;

	mBuffersStorage->destroyFrameBuffer(fbReq->fb_cookie);
}

void DisplayCommandHandler::setConfig(const xendispl_req& req)
{
	const xendispl_set_config_req* configReq = &req.op.set_config;

	DLOG(mLog, DEBUG) << "Handle command [SET CONFIG], FB cookie: "
					  << hex << setfill('0') << setw(16)
					  << configReq->fb_cookie;

	if (configReq->fb_cookie != 0)
	{
		mConnector->init(configReq->x, configReq->y,
						 configReq->width, configReq->height,
						 mBuffersStorage->getFrameBuffer(configReq->fb_cookie));
	}
	else
	{
		mConnector->release();
	}
}

void DisplayCommandHandler::sendFlipEvent(uint64_t fbCookie)
{
	DLOG(mLog, DEBUG) << "Event [PAGE FLIP], conn ID: "
					  << mConnector->getId() << ", fb ID: "
					  << hex << setfill('0') << setw(16) << fbCookie;

	xendispl_evt event {};

	event.type = XENDISPL_EVT_PG_FLIP;
	event.op.pg_flip.fb_cookie = fbCookie;
	event.id = mEventId++;

	mEventBuffer->sendEvent(event);
}
