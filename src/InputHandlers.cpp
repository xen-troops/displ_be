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

#include "InputHandlers.hpp"

using std::bind;

using namespace std::placeholders;

using InputItf::KeyboardPtr;
using InputItf::PointerPtr;
using InputItf::TouchPtr;

/*******************************************************************************
 * KeyboardHandler
 ******************************************************************************/

KeyboardHandler::KeyboardHandler(KeyboardPtr keyboard,
								 EventRingBufferPtr ringBuffer) :
	mKeyboard(keyboard),
	mRingBuffer(ringBuffer),
	mLog("KeyboardHandler")
{
	mKeyboard->setCallbacks({bind(&KeyboardHandler::onKey, this, _1, _2)});
}

void KeyboardHandler::onKey(uint32_t key, uint32_t state)
{
	DLOG(mLog, DEBUG) << "onKey key: " << key << ", state: " << state;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_KEY;
	event.key.keycode = key;
	event.key.pressed = state;

	mRingBuffer->sendEvent(event);
}

/*******************************************************************************
 * PointerHandler
 ******************************************************************************/

PointerHandler::PointerHandler(PointerPtr pointer,
							   EventRingBufferPtr ringBuffer) :
	mPointer(pointer),
	mRingBuffer(ringBuffer),
	mLog("PointerHandler")
{
	mPointer->setCallbacks({
		nullptr, // relative move
		bind(&PointerHandler::onMove, this, _1, _2),
		bind(&PointerHandler::onButton, this, _1, _2),
		bind(&PointerHandler::onAxis, this, _1, _2),
	});
}

void PointerHandler::onMove(int32_t x, int32_t y)
{
	DLOG(mLog, DEBUG) << "onMove x: " << x << ", y: " << y;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_POS;
	event.pos.abs_x = x;
	event.pos.abs_y = y;

	mRingBuffer->sendEvent(event);
}

void PointerHandler::onButton(uint32_t button, uint32_t state)
{
	DLOG(mLog, DEBUG) << "onButton button: " << button << ", state: " << state;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_KEY;
	event.key.keycode = button;
	event.key.pressed = state;

	mRingBuffer->sendEvent(event);
}

void PointerHandler::onAxis(uint32_t axis, int32_t value)
{
	DLOG(mLog, DEBUG) << "onAxis axis: " << axis << ", value: " << value;

	// protocol supports only vertical axis
	if (axis == 0)
	{
		xenkbd_in_event event = {};

		event.type = XENKBD_TYPE_MOTION;
		event.motion.rel_z = value;

		mRingBuffer->sendEvent(event);
	}
}

/*******************************************************************************
 * TouchHandler
 ******************************************************************************/

TouchHandler::TouchHandler(TouchPtr touch, EventRingBufferPtr ringBuffer) :
	mTouch(touch),
	mRingBuffer(ringBuffer),
	mLog("TouchHandler")
{
	mTouch->setCallbacks({
		bind(&TouchHandler::onDown, this, _1, _2, _3),
		bind(&TouchHandler::onUp, this, _1),
		bind(&TouchHandler::onMotion, this, _1, _2, _3),
		bind(&TouchHandler::onFrame, this),
	});
}

void TouchHandler::onDown(int32_t id, int32_t x, int32_t y)
{
	DLOG(mLog, DEBUG) << "onDown id: " << id << ", x: " << x << ", y: " << y;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_DOWN;
	event.mtouch.contact_id = id;
	event.mtouch.u.pos = {x, y};

	mRingBuffer->sendEvent(event);
}

void TouchHandler::onUp(int32_t id)
{
	DLOG(mLog, DEBUG) << "onUp id: " << id;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_UP;
	event.mtouch.contact_id = id;

	mRingBuffer->sendEvent(event);

	onFrame();
}

void TouchHandler::onMotion(int32_t id, int32_t x, int32_t y)
{
	DLOG(mLog, DEBUG) << "onMotion id: " << id << ", x: " << x << ", y: " << y;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_MOTION;
	event.mtouch.contact_id = id;
	event.mtouch.u.pos = {x, y};

	mRingBuffer->sendEvent(event);
}

void TouchHandler::onFrame()
{
	DLOG(mLog, DEBUG) << "onFrame";

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_SYN;

	mRingBuffer->sendEvent(event);
}
