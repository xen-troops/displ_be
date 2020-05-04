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

#ifndef SRC_DISPLAYCOMMANDHANDLER_HPP_
#define SRC_DISPLAYCOMMANDHANDLER_HPP_

#include <cstdint>
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
class EventRingBuffer : public XenBackend::RingBufferOutBase<
		xendispl_event_page, xendispl_evt>
{
public:
	/**
	 * @param conIndex  connector index
	 * @param domId     frontend domain id
	 * @param port      event channel port number
	 * @param ref       grant table reference
	 * @param offset    start of the ring buffer inside the page
	 * @param size      size of the ring buffer
	 */
	EventRingBuffer(int conIndex, domid_t domId, evtchn_port_t port,
					grant_ref_t ref, int offset, size_t size);

private:
	int mConIndex;
	XenBackend::Log mLog;
};

typedef std::shared_ptr<EventRingBuffer> EventRingBufferPtr;

/**
 * Handles commands received from the frontend.
 * @ingroup displ_be
 */
class DisplayCommandHandler
{
public:
	/**
	 * @param display        display object
	 * @param connector      connector object
	 * @param buffersStorage buffers storage
	 * @param eventBuffer    event ring buffer
	 */
	DisplayCommandHandler(DisplayItf::DisplayPtr display,
						  DisplayItf::ConnectorPtr connector,
						  BuffersStoragePtr buffersStorage,
						  EventRingBufferPtr eventBuffer);
	~DisplayCommandHandler();

	/**
	 * Processes commands received from the frontend.
	 * @param req frontend request
	 * @param rsp backend response
	 * @return status
	 */
	int processCommand(const xendispl_req& req, xendispl_resp& rsp);

private:
	typedef void(DisplayCommandHandler::*CommandFn)(const xendispl_req& req,
													xendispl_resp& rsp);

	static std::unordered_map<int, CommandFn> sCmdTable;

	DisplayItf::DisplayPtr mDisplay;
	DisplayItf::ConnectorPtr mConnector;
	BuffersStoragePtr mBuffersStorage;
	EventRingBufferPtr mEventBuffer;
	uint16_t mEventId;

	XenBackend::Log mLog;

	void pageFlip(const xendispl_req& req, xendispl_resp& rsp);
	void createDisplayBuffer(const xendispl_req& req, xendispl_resp& rsp);
	void destroyDisplayBuffer(const xendispl_req& req, xendispl_resp& rsp);
	void attachFrameBuffer(const xendispl_req& req, xendispl_resp& rsp);
	void detachFrameBuffer(const xendispl_req& req, xendispl_resp& rsp);
	void setConfig(const xendispl_req& req, xendispl_resp& rsp);
	void getEDID(const xendispl_req& req, xendispl_resp& rsp);

	void sendFlipEvent(uint64_t fbCookie);
};

#endif /* SRC_DISPLAYCOMMANDHANDLER_HPP_ */
