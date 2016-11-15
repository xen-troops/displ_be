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

#ifndef SRC_COMMANDHANDLER_HPP_
#define SRC_COMMANDHANDLER_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include <xen/be/XenGnttab.hpp>
#include <xen/be/Log.hpp>

extern "C" {
#include <xen/io/drmif_linux.h>
}

#include "Drm.hpp"

class ConEventRingBuffer;

/**
 * Handles commands received from the frontend.
 * @ingroup drm_be
 */
class CommandHandler
{
public:
	/**
	 * @param connectorId connector id
	 * @param domId domain id
	 */
	CommandHandler(uint32_t connectorId, int domId, Drm::DrmDevice& drm,
				   std::shared_ptr<ConEventRingBuffer> eventBuffer);
	~CommandHandler();

	/**
	 * Processes commands received from the frontend.
	 * @param req
	 * @return
	 */
	uint8_t processCommand(const xendrm_req& req);

private:
	typedef void(CommandHandler::*CommandFn)(const xendrm_req& req);

	static CommandFn sCmdTable[];

	uint32_t mRemoteConnectorId;
	int mDomId;
	std::shared_ptr<ConEventRingBuffer> mEventBuffer;

	Drm::DrmDevice& mDrm;

	uint32_t mLocalConnectorId;
	uint32_t mCrtcId;

	XenBackend::Log mLog;

	struct DumbBuffer
	{
		Drm::Dumb& dumb;
		std::unique_ptr<XenBackend::XenGnttabBuffer> buffer;
	};

	std::map<uint32_t, Drm::FrameBuffer&> mFrameBuffers;
	std::map<uint32_t, DumbBuffer> mDumbBuffers;

	void pageFlip(const xendrm_req& req);
	void createDumb(const xendrm_req& req);
	void destroyDumb(const xendrm_req& req);
	void createFrameBuffer(const xendrm_req& req);
	void destroyFrameBuffer(const xendrm_req& req);
	void setConfig(const xendrm_req& req);

	void getBufferRefs(grant_ref_t startDirectory,
					   std::vector<grant_ref_t>& refs);

	uint32_t getLocalConnectorId();
	Drm::Dumb& getLocalDumb(uint32_t handle);
	Drm::FrameBuffer& getLocalFb(uint32_t fbId);
	void copyBuffer(uint32_t fbId);
	void sendFlipEvent(uint8_t crtcIdx, uint32_t fb_id);
};

#endif /* SRC_COMMANDHANDLER_HPP_ */
