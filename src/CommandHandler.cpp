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

#include "CommandHandler.hpp"

#include <cstddef>
#include <cstring>
#include <sys/mman.h>

using std::make_pair;
using std::memcpy;
using std::min;
using std::move;
using std::out_of_range;
using std::shared_ptr;
using std::to_string;
using std::unordered_map;
using std::vector;

using XenBackend::XenException;
using XenBackend::XenGnttabBuffer;

unordered_map<int, CommandHandler::CommandFn> CommandHandler::sCmdTable =
{
	{XENDISPL_OP_PG_FLIP,		&CommandHandler::pageFlip},
	{XENDISPL_OP_FB_ATTACH,		&CommandHandler::attachFrameBuffer},
	{XENDISPL_OP_FB_DETACH,		&CommandHandler::detachFrameBuffer},
	{XENDISPL_OP_DBUF_CREATE,	&CommandHandler::createDisplayBuffer},
	{XENDISPL_OP_DBUF_DESTROY,	&CommandHandler::destroyDisplayBuffer},
	{XENDISPL_OP_SET_CONFIG,	&CommandHandler::setConfig}
};

/*******************************************************************************
 * ConEventRingBuffer
 ******************************************************************************/

ConEventRingBuffer::ConEventRingBuffer(int id, int domId, int port,
									   int ref, int offset, size_t size) :
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

CommandHandler::CommandHandler(shared_ptr<DisplayItf> display,
							   uint32_t conId, int domId,
							   shared_ptr<ConEventRingBuffer> eventBuffer) :
	mDomId(domId),
	mEventBuffer(eventBuffer),
	mDisplay(display),
	mConnector(mDisplay->getConnectorById(conId)),
	mLog("CommandHandler")
{
	LOG(mLog, DEBUG) << "Create command handler, dom: " << mDomId;
}

CommandHandler::~CommandHandler()
{
	LOG(mLog, DEBUG) << "Delete command handler, dom: " << mDomId;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

uint8_t CommandHandler::processCommand(const xendispl_req& req)
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
	catch(const DisplayItfException& e)
	{
		LOG(mLog, ERROR) << e.what();

		status = XENDISPL_RSP_ERROR;
	}

	DLOG(mLog, DEBUG) << "Return status: [" << static_cast<int>(status) << "]";

	return status;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void CommandHandler::pageFlip(const xendispl_req& req)
{
	xendispl_page_flip_req flipReq = req.op.pg_flip;

	DLOG(mLog, DEBUG) << "Handle command [PAGE FLIP], fb ID: "
					  << flipReq.fb_cookie << ", conn idx: "
					  << static_cast<int>(flipReq.conn_idx);

	auto frameBuffer = getLocalFb(flipReq.fb_cookie);

	copyBuffer(flipReq.fb_cookie);

	mConnector->pageFlip(frameBuffer, [flipReq, this] ()
			{ sendFlipEvent(flipReq.conn_idx, flipReq.fb_cookie); });
}

void CommandHandler::createDisplayBuffer(const xendispl_req& req)
{
	const xendispl_dbuf_create_req* dbufReq = &req.op.dbuf_create;

	DLOG(mLog, DEBUG) << "Handle command [CREATE DBUF], handle: "
					  << dbufReq->dbuf_cookie;

	LocalDisplayBuffer displayBuffer {
		mDisplay->createDisplayBuffer(dbufReq->width,
									  dbufReq->height,
									  dbufReq->bpp) };

	vector<grant_ref_t> refs;

	getBufferRefs(dbufReq->gref_directory_start, dbufReq->buffer_sz, refs);

	displayBuffer.buffer.reset(new XenGnttabBuffer(mDomId, refs.data(),
												   refs.size()));

	mDisplayBuffers.emplace(dbufReq->dbuf_cookie, move(displayBuffer));
}

void CommandHandler::destroyDisplayBuffer(const xendispl_req& req)
{
	const xendispl_dbuf_destroy_req* dbufReq = &req.op.dbuf_destroy;

	DLOG(mLog, DEBUG) << "Handle command [DESTROY DBUF], handle: "
					  << dbufReq->dbuf_cookie;

	mDisplayBuffers.erase(dbufReq->dbuf_cookie);
}

void CommandHandler::attachFrameBuffer(const xendispl_req& req)
{
	const xendispl_fb_attach_req* fbReq = &req.op.fb_attach;

	DLOG(mLog, DEBUG) << "Handle command [ATTACH FB], handle: "
					  << fbReq->fb_cookie << ", id: " << fbReq->fb_cookie;

	auto frameBuffer =
			mDisplay->createFrameBuffer(getLocalDb(fbReq->dbuf_cookie),
									    fbReq->width, fbReq->height,
									    fbReq->pixel_format);

	mFrameBuffers.emplace(fbReq->fb_cookie, frameBuffer);
}

void CommandHandler::detachFrameBuffer(const xendispl_req& req)
{
	const xendispl_fb_detach_req* fbReq = &req.op.fb_detach;

	DLOG(mLog, DEBUG) << "Handle command [DETACH FB], id: " << fbReq->fb_cookie;

	mFrameBuffers.erase(fbReq->fb_cookie);
}

void CommandHandler::setConfig(const xendispl_req& req)
{
	const xendispl_set_config_req* configReq = &req.op.set_config;

	DLOG(mLog, DEBUG) << "Handle command [SET CONFIG], fb ID: "
					  << configReq->fb_cookie;

	if (configReq->fb_cookie != 0)
	{
		mConnector->init(configReq->x, configReq->y,
						 configReq->width, configReq->height,
						 getLocalFb(configReq->fb_cookie));
	}
	else
	{
		mConnector->release();
	}
}

void CommandHandler::getBufferRefs(grant_ref_t startDirectory, uint32_t size,
								   vector<grant_ref_t>& refs)
{
	refs.clear();

	size_t requestedNumGrefs = (size + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE;

	DLOG(mLog, DEBUG) << "Get buffer refs, directory: " << startDirectory
					  << ", size: " << size
					  << ", in grefs: " << requestedNumGrefs;


	while(startDirectory != 0)
	{

		XenGnttabBuffer pageBuffer(mDomId, startDirectory);

		xendispl_page_directory* pageDirectory =
				static_cast<xendispl_page_directory*>(pageBuffer.get());

		size_t numGrefs = min(requestedNumGrefs, XC_PAGE_SIZE -
							  offsetof(xendispl_page_directory, gref));

		refs.insert(refs.end(), pageDirectory->gref,
					pageDirectory->gref + numGrefs);

		requestedNumGrefs -= numGrefs;

		startDirectory = pageDirectory->gref_dir_next_page;
	}

	DLOG(mLog, DEBUG) << "Get buffer refs, num refs: " << refs.size();
}

shared_ptr<DisplayBufferItf> CommandHandler::getLocalDb(uint64_t cookie)
{
	auto iter = mDisplayBuffers.find(cookie);

	if (iter == mDisplayBuffers.end())
	{
		throw DisplayItfException("Dumb cookie not found");
	}

	return iter->second.displayBuffer;
}

std::shared_ptr<FrameBufferItf> CommandHandler::getLocalFb(uint64_t cookie)
{
	auto iter = mFrameBuffers.find(cookie);

	if (iter == mFrameBuffers.end())
	{
		throw DisplayItfException("Frame buffer cookie not found");
	}

	return iter->second;
}

void CommandHandler::copyBuffer(uint64_t cookie)
{
	auto displayBuffer = getLocalFb(cookie)->getDisplayBuffer();

	auto iter = mDisplayBuffers.begin();

	for (; iter != mDisplayBuffers.end(); iter++)
	{
		if (displayBuffer == iter->second.displayBuffer)
		{
			memcpy(displayBuffer->getBuffer(), iter->second.buffer->get(),
				   displayBuffer->getSize());

			return;
		}
	}

	throw DisplayItfException("Handle not found");
}

void CommandHandler::sendFlipEvent(uint8_t conIdx, uint64_t fb_cookie)
{
	DLOG(mLog, DEBUG) << "Event [PAGE FLIP], conn idx: "
					  << static_cast<int>(conIdx);

	xendispl_evt event {};

	event.op.pg_flip.conn_idx = conIdx;
	event.op.pg_flip.fb_cookie = fb_cookie;

	mEventBuffer->sendEvent(event);
}
