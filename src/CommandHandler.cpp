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

#include <cstring>
#include <sys/mman.h>

#include "DrmBackend.hpp"
#include "drm/Exception.hpp"

using std::shared_ptr;
using std::make_pair;
using std::memcpy;
using std::move;
using std::to_string;
using std::vector;

using XenBackend::XenException;
using XenBackend::XenGnttabBuffer;

using Drm::Connector;
using Drm::Device;
using Drm::DrmException;
using Drm::Dumb;
using Drm::FrameBuffer;
using Drm::cInvalidId;

CommandHandler::CommandFn CommandHandler::sCmdTable[] =
{
	&CommandHandler::pageFlip,
	&CommandHandler::createDumb,
	&CommandHandler::destroyDumb,
	&CommandHandler::createFrameBuffer,
	&CommandHandler::destroyFrameBuffer,
	&CommandHandler::setConfig,
};

/***************************************************************************//**
 * CommandHandler
 ******************************************************************************/

CommandHandler::CommandHandler(uint32_t connectorId, int domId, Device& drm,
							   shared_ptr<ConEventRingBuffer> eventBuffer) :
	mRemoteConnectorId(connectorId),
	mDomId(domId),
	mDrm(drm),
	mLocalConnectorId(cInvalidId),
	mCrtcId(cInvalidId),
	mLog("CommandHandler " + to_string(connectorId))
{
	LOG(mLog, DEBUG) << "Create command handler, dom: " << mDomId;
}

CommandHandler::~CommandHandler()
{
	LOG(mLog, DEBUG) << "Delete command handler, dom: " << mDomId;
}

/***************************************************************************//**
 * Public
 ******************************************************************************/

uint8_t CommandHandler::processCommand(const xendrm_req& req)
{
	uint8_t status = XENDRM_RSP_OKAY;

	try
	{
		if (req.u.data.operation < (sizeof(sCmdTable) / sizeof(CommandFn)))
		{
			(this->*sCmdTable[req.u.data.operation])(req);
		}
		else
		{
			status = XENDRM_RSP_ERROR;
		}
	}
	catch(const DrmException& e)
	{
		LOG(mLog, ERROR) << e.what();

		status = XENDRM_RSP_ERROR;
	}

	DLOG(mLog, DEBUG) << "Return status: [" << static_cast<int>(status) << "]";

	return status;
}

/***************************************************************************//**
 * Private
 ******************************************************************************/

void CommandHandler::pageFlip(const xendrm_req& req)
{
	xendrm_page_flip_req flipReq = req.u.data.op.pg_flip;

	DLOG(mLog, DEBUG) << "Handle command [PAGE FLIP], fb ID: "
					  << flipReq.fb_id << ", crtc idx: " << flipReq.crtc_idx;

	FrameBuffer& frameBuffer = getLocalFb(flipReq.fb_id);

	copyBuffer(flipReq.fb_id);

	frameBuffer.pageFlip(mCrtcId, [flipReq, this] ()
								  { sendFlipEvent(flipReq.crtc_idx,
								  flipReq.fb_id); });
}

void CommandHandler::createDumb(const xendrm_req& req)
{
	const xendrm_dumb_create_req* dumbReq = &req.u.data.op.dumb_create;

	DLOG(mLog, DEBUG) << "Handle command [CREATE DUMB], handle: "
					  << dumbReq->handle;

	DumbBuffer dumbBuffer { mDrm.createDumb(dumbReq->width, dumbReq->height,
											dumbReq->bpp) };

	vector<grant_ref_t> refs;

	getBufferRefs(dumbReq->gref_directory_start, refs);

	dumbBuffer.buffer.reset(new XenGnttabBuffer(mDomId, refs.data(),
												refs.size(),
												PROT_READ | PROT_WRITE));

	mDumbBuffers.emplace(dumbReq->handle, move(dumbBuffer));
}

void CommandHandler::destroyDumb(const xendrm_req& req)
{
	const xendrm_dumb_destroy_req* dumbReq = &req.u.data.op.dumb_destroy;

	DLOG(mLog, DEBUG) << "Handle command [DESTROY DUMB], handle: "
					  << dumbReq->handle;

	mDrm.deleteDumb(getLocalDumb(dumbReq->handle).getHandle());

	mDumbBuffers.erase(dumbReq->handle);
}

void CommandHandler::createFrameBuffer(const xendrm_req& req)
{
	const xendrm_fb_create_req* fbReq = &req.u.data.op.fb_create;

	DLOG(mLog, DEBUG) << "Handle command [CREATE FB], handle: "
					  << fbReq->handle << ", id: " << fbReq->fb_id;

	FrameBuffer& frameBuffer =
			mDrm.createFrameBuffer(getLocalDumb(fbReq->handle),
								   fbReq->width, fbReq->height,
								   fbReq->pixel_format);

	mFrameBuffers.emplace(fbReq->fb_id, frameBuffer);
}

void CommandHandler::destroyFrameBuffer(const xendrm_req& req)
{
	const xendrm_fb_destroy_req* fbReq = &req.u.data.op.fb_destroy;

	DLOG(mLog, DEBUG) << "Handle command [DELETE FB], id: " << fbReq->fb_id;

	mDrm.deleteFrameBuffer(getLocalFb(fbReq->fb_id).getId());

	mFrameBuffers.erase(fbReq->fb_id);
}

void CommandHandler::setConfig(const xendrm_req& req)
{
	const xendrm_set_config_req* configReq = &req.u.data.op.set_config;

	DLOG(mLog, DEBUG) << "Handle command [SET CONFIG], fb ID: "
					  << configReq->fb_id;

	if (configReq->fb_id != cInvalidId)
	{
		Connector& connector = mDrm.getConnectorById(getLocalConnectorId());

		connector.init(configReq->x, configReq->y,
					   configReq->width, configReq->height,
					   configReq->bpp, getLocalFb(configReq->fb_id).getId());

		mLocalConnectorId = connector.getId();
		mCrtcId = connector.getCrtcId();
	}
	else
	{
		if (mLocalConnectorId != cInvalidId)
		{
			mDrm.getConnectorById(getLocalConnectorId()).release();
			mCrtcId = cInvalidId;
		}
	}
}

void CommandHandler::getBufferRefs(grant_ref_t startDirectory,
								   vector<grant_ref_t>& refs)
{
	refs.clear();

	do
	{

		XenGnttabBuffer pageBuffer(mDomId, startDirectory,
								   PROT_READ | PROT_WRITE);
		xendrm_page_directory* pageDirectory =
				static_cast<xendrm_page_directory*>(pageBuffer.get());

		DLOG(mLog, DEBUG) << "Get buffer refs, directory: " << startDirectory
						  << ", num refs: " << pageDirectory->num_grefs;

		refs.insert(refs.end(), pageDirectory->gref,
					pageDirectory->gref + pageDirectory->num_grefs);

		startDirectory = pageDirectory->gref_dir_next_page;

	}
	while(startDirectory != 0);

	DLOG(mLog, DEBUG) << "Get buffer refs, num refs: " << refs.size();
}

uint32_t CommandHandler::getLocalConnectorId()
{
	for (size_t i = 0; i < mDrm.getConnectorsCount(); i++)
	{
		Connector& connector = mDrm.getConnectorByIndex(i);
		if (connector.isConnected() && !connector.isInitialized())
		{
			return connector.getId();
		}
	}

	throw DrmException("No available connectors found");
}

Dumb& CommandHandler::getLocalDumb(uint32_t handle)
{
	auto iter = mDumbBuffers.find(handle);

	if (iter == mDumbBuffers.end())
	{
		throw DrmException("Handle not found");
	}

	return iter->second.dumb;
}

Drm::FrameBuffer& CommandHandler::getLocalFb(uint32_t fbId)
{
	auto iter = mFrameBuffers.find(fbId);

	if (iter == mFrameBuffers.end())
	{
		throw DrmException("Frame buffer ID not found");
	}

	return iter->second;
}

void CommandHandler::copyBuffer(uint32_t fbId)
{
	auto handle = getLocalFb(fbId).getDumb().getHandle();

	for (auto iter = mDumbBuffers.begin(); iter != mDumbBuffers.end(); iter++)
	{
		Dumb& dumb = iter->second.dumb;

		if (handle == dumb.getHandle())
		{
			memcpy(dumb.getBuffer(), iter->second.buffer->get(),
				   dumb.getSize());

			return;
		}
	}

	throw DrmException("Handle not found");
}

void CommandHandler::sendFlipEvent(uint8_t crtcIdx, uint32_t fb_id)
{
	DLOG(mLog, DEBUG) << "Event [PAGE FLIP], crtc idx: " << crtcIdx;

	xendrm_evt event {};

	event.u.data.op.pg_flip.crtc_idx = crtcIdx;
	event.u.data.op.pg_flip.fb_id = fb_id;

	mEventBuffer->sendEvent(event);
}
