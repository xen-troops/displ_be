/*
 *  Input handlers
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

#ifndef SRC_INPUTHANDLERS_HPP_
#define SRC_INPUTHANDLERS_HPP_

#include <cstdint>

#include <xen/be/Log.hpp>
#include <xen/be/RingBufferBase.hpp>

#include <xen/io/kbdif.h>

#include "InputItf.hpp"

/***************************************************************************//**
 * Ring buffer used to send events to the frontend.
 * @ingroup input_be
 ******************************************************************************/
class EventRingBuffer : public XenBackend::RingBufferOutBase<xenkbd_page,
															 xenkbd_in_event>
{
public:
	/**
	 * @param domId     frontend domain id
	 * @param port      event channel port number
	 * @param ref       grant table reference
	 * @param offset    start of the ring buffer inside the page
	 * @param size      size of the ring buffer
	 */
	EventRingBuffer(domid_t domId, evtchn_port_t port, int ref,
					int offset, size_t size) :
		RingBufferOutBase<xenkbd_page, xenkbd_in_event>(domId, port, ref,
														offset, size) {}
};

typedef std::shared_ptr<EventRingBuffer> EventRingBufferPtr;

class KeyboardHandler
{
public:

	KeyboardHandler(InputItf::KeyboardPtr keyboard,
					EventRingBufferPtr ringBuffer);

private:

	InputItf::KeyboardPtr mKeyboard;
	EventRingBufferPtr mRingBuffer;
	XenBackend::Log mLog;

	void onKey(uint32_t key, uint32_t state);
};

class PointerHandler
{
public:

	PointerHandler(InputItf::PointerPtr pointer,
				   EventRingBufferPtr ringBuffer);

private:

	InputItf::PointerPtr mPointer;
	EventRingBufferPtr mRingBuffer;
	XenBackend::Log mLog;

	void onMove(int32_t x, int32_t y);
	void onButton(uint32_t button, uint32_t state);
	void onAxis(uint32_t axis, int32_t value);
};

class TouchHandler
{
public:

	TouchHandler(InputItf::TouchPtr touch, EventRingBufferPtr ringBuffer);

private:

	InputItf::TouchPtr mTouch;
	EventRingBufferPtr mRingBuffer;
	XenBackend::Log mLog;

	void onDown(int32_t id, int32_t x, int32_t y);
	void onUp(int32_t id);
	void onMotion(int32_t id, int32_t x, int32_t y);
	void onFrame();
};

#endif /* SRC_INPUTHANDLERS_HPP_ */
