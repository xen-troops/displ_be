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
#include <unordered_map>
#include <vector>

#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

#include "BuffersStorage.hpp"
#include "DisplayItf.hpp"

/***************************************************************************//**
 * Ring buffer used to send events to the frontend.
 * @ingroup displ_be
 ******************************************************************************/
class ConEventRingBuffer : public XenBackend::RingBufferOutBase<
											  xendispl_event_page, xendispl_evt>
{
public:
	/**
	 * @param id        connector id
	 * @param domId     frontend domain id
	 * @param port      event channel port number
	 * @param ref       grant table reference
	 * @param offset    start of the ring buffer inside the page
	 * @param size      size of the ring buffer
	 */
	ConEventRingBuffer(int id, int domId, int port,
					   int ref, int offset, size_t size);

private:
	int mId;
	XenBackend::Log mLog;
};

/**
 * Handles commands received from the frontend.
 * @ingroup displ_be
 */
class CommandHandler
{
public:
	/**
	 * @param connector      connector object
	 * @param buffersStorage buffers storage
	 * @param eventBuffer    event ring buffer
	 */
	CommandHandler(std::shared_ptr<ConnectorItf> connector,
				   std::shared_ptr<BuffersStorage> buffersStorage,
				   std::shared_ptr<ConEventRingBuffer> eventBuffer);
	~CommandHandler();

	/**
	 * Processes commands received from the frontend.
	 * @param req frontend request
	 * @return
	 */
	uint8_t processCommand(const xendispl_req& req);

private:
	typedef void(CommandHandler::*CommandFn)(const xendispl_req& req);

	static std::unordered_map<int, CommandFn> sCmdTable;

	std::shared_ptr<ConnectorItf> mConnector;
	std::shared_ptr<BuffersStorage> mBuffersStorage;
	std::shared_ptr<ConEventRingBuffer> mEventBuffer;

	XenBackend::Log mLog;

	void pageFlip(const xendispl_req& req);
	void createDisplayBuffer(const xendispl_req& req);
	void destroyDisplayBuffer(const xendispl_req& req);
	void attachFrameBuffer(const xendispl_req& req);
	void detachFrameBuffer(const xendispl_req& req);
	void setConfig(const xendispl_req& req);

	void sendFlipEvent(uint8_t conIdx, uint64_t fbCookie);
};

#endif /* SRC_COMMANDHANDLER_HPP_ */
