/*
 *  Input backend
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

#include "InputBackend.hpp"

#include <sstream>
#include <vector>

#include "input/DevInput.hpp"
#ifdef WITH_WAYLAND
#include "input/WlInput.hpp"
#endif

using std::bind;
using std::istringstream;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::toupper;
using std::transform;
using std::vector;

using namespace std::placeholders;

using XenBackend::BackendBase;
using XenBackend::FrontendHandlerPtr;
using XenBackend::Log;
using XenBackend::RingBufferPtr;

using InputItf::KeyboardCallbacks;
using InputItf::KeyboardPtr;
using InputItf::PointerCallbacks;
using InputItf::PointerPtr;
using InputItf::TouchCallbacks;
using InputItf::TouchPtr;

/*******************************************************************************
 * InputRingBuffer
 ******************************************************************************/

InputRingBuffer::InputRingBuffer(KeyboardPtr keyboard, PointerPtr pointer,
								 TouchPtr touch, bool isReqAbs,
								 bool isReqMTouch, domid_t domId,
								 evtchn_port_t port, int ref,
								 int offset, size_t size) :
	RingBufferOutBase<xenkbd_page, xenkbd_in_event>(domId, port, ref,
													offset, size),
	mKeyboard(keyboard),
	mPointer(pointer),
	mTouch(touch),
	mLog("InputRingBuffer")
{
	if (mKeyboard)
	{
		mKeyboard->setCallbacks({bind(&InputRingBuffer::onKey, this, _1, _2)});
	}

	if (mPointer)
	{
		if (isReqAbs)
		{
			mPointer->setCallbacks({
				bind(&InputRingBuffer::onMoveRel, this, _1, _2, _3),
				bind(&InputRingBuffer::onMoveAbs, this, _1, _2, _3),
				bind(&InputRingBuffer::onButton, this, _1, _2),
			});
		}
		else
		{
			mPointer->setCallbacks({
				bind(&InputRingBuffer::onMoveRel, this, _1, _2, _3),
				nullptr,
				bind(&InputRingBuffer::onButton, this, _1, _2),
			});
		}
	}

	if (mTouch && isReqMTouch)
	{
		mTouch->setCallbacks({
			bind(&InputRingBuffer::onDown, this, _1, _2, _3),
			bind(&InputRingBuffer::onUp, this, _1),
			bind(&InputRingBuffer::onMotion, this, _1, _2, _3),
			bind(&InputRingBuffer::onFrame, this, _1),
		});
	}

	LOG(mLog, DEBUG) << "Create, reqAbs: " << isReqAbs
					 << ", reqMTouch: " << isReqMTouch;
}

InputRingBuffer::~InputRingBuffer()
{
	LOG(mLog, DEBUG) << "Delete";
}

void InputRingBuffer::onKey(uint32_t key, uint32_t state)
{
	DLOG(mLog, DEBUG) << "onKey key: " << key << ", state: " << state;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_KEY;
	event.key.keycode = key;
	event.key.pressed = state;

	sendEvent(event);
}

void InputRingBuffer::onMoveRel(int32_t x, int32_t y, int32_t z)
{
	DLOG(mLog, DEBUG) << "onMoveRel x: " << x << ", y: " << y << ", z: " << z;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MOTION;
	event.motion.rel_x = x;
	event.motion.rel_y = y;
	event.motion.rel_z = z;

	sendEvent(event);
}

void InputRingBuffer::onMoveAbs(int32_t x, int32_t y, int32_t z)
{
	DLOG(mLog, DEBUG) << "onMoveAbs x: " << x << ", y: " << y << ", z: " << z;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_POS;
	event.pos.abs_x = x;
	event.pos.abs_y = y;
	event.pos.rel_z = z;

	sendEvent(event);
}

void InputRingBuffer::onButton(uint32_t button, uint32_t state)
{
	DLOG(mLog, DEBUG) << "onButton button: " << button << ", state: " << state;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_KEY;
	event.key.keycode = button;
	event.key.pressed = state;

	sendEvent(event);
}

void InputRingBuffer::onDown(int32_t id, int32_t x, int32_t y)
{
	DLOG(mLog, DEBUG) << "onDown id: " << id << ", x: " << x << ", y: " << y;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_DOWN;
	event.mtouch.contact_id = id;
	event.mtouch.u.pos = {x, y};

	sendEvent(event);
}

void InputRingBuffer::onUp(int32_t id)
{
	DLOG(mLog, DEBUG) << "onUp id: " << id;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_UP;
	event.mtouch.contact_id = id;

	sendEvent(event);
}

void InputRingBuffer::onMotion(int32_t id, int32_t x, int32_t y)
{
	DLOG(mLog, DEBUG) << "onMotion id: " << id << ", x: " << x << ", y: " << y;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_MOTION;
	event.mtouch.contact_id = id;
	event.mtouch.u.pos = {x, y};

	sendEvent(event);
}

void InputRingBuffer::onFrame(int32_t id)
{
	DLOG(mLog, DEBUG) << "onFrame id: " << id;

	xenkbd_in_event event = {};

	event.type = XENKBD_TYPE_MTOUCH;
	event.mtouch.event_type = XENKBD_MT_EV_SYN;
	event.mtouch.contact_id = id;

	sendEvent(event);
}


/*******************************************************************************
 * InputFrontendHandler
 ******************************************************************************/

InputFrontendHandler::InputFrontendHandler(const string& devName,
										   domid_t beDomId, domid_t feDomId,
										   uint16_t devId) :
	FrontendHandlerBase("VkbdFrontend", devName, beDomId, feDomId, devId),
	mLog("VkbdFrontend")
{
	setBackendState(XenbusStateInitWait);
}

void InputFrontendHandler::onBind()
{
	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	evtchn_port_t port = getXenStore().readUint(
			getXsFrontendPath() + "/" XENKBD_FIELD_EVT_CHANNEL);
	grant_ref_t ref = getXenStore().readUint(
			getXsFrontendPath() + "/" XENKBD_FIELD_RING_GREF);

	auto id = getXenStore().readString(getXsFrontendPath() + "/id");


	bool isReqAbs = false;
	bool isReqMTouch = false;

	string reqAbsPath = getXsBackendPath() + "/" XENKBD_FIELD_REQ_ABS_POINTER;
	string reqMTouchPath = getXsFrontendPath() + "/" XENKBD_FIELD_REQ_MTOUCH;

	if (getXenStore().checkIfExist(reqAbsPath))
	{
		isReqAbs = getXenStore().readInt(reqAbsPath);
	}

	if (getXenStore().checkIfExist(reqMTouchPath))
	{
		isReqMTouch = getXenStore().readInt(reqMTouchPath);
	}

	string keyboardId, pointerId, touchId;

	parseInputId(id, keyboardId, pointerId, touchId);

	InputRingBufferPtr eventRingBuffer(
			new InputRingBuffer(createInputDevice<KeyboardCallbacks>(keyboardId),
								createInputDevice<PointerCallbacks>(pointerId),
								createInputDevice<TouchCallbacks>(touchId),
								isReqAbs, isReqMTouch,
								getDomId(), port, ref,
								XENKBD_IN_RING_OFFS, XENKBD_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);
}

void InputFrontendHandler::parseInputId(const string& id, string& keyboardId,
										string& pointerId, string& touchId)
{
	istringstream input(id);
	string token;

	LOG(mLog, DEBUG) << "Parse input id: " << id;

	while (getline(input, token, ';'))
	{
		auto pos = token.find(":");
		auto key = token.substr(0, pos);
		auto val =  token.substr(++pos);

		transform(key.begin(), key.end(), key.begin(), (int (*)(int))toupper);

		if (key == "K")
		{
			keyboardId = val;
		}
		else if (key == "P")
		{
			pointerId = val;
		}
		else if (key == "T")
		{
			touchId = val;
		}
		else
		{
			throw XenBackend::Exception("Invalid key: " + key + " in id",
										EINVAL);
		}
	}
}

template<typename T>
shared_ptr<InputItf::InputDevice<T>>
InputFrontendHandler::createInputDevice(const string& id)
{
	if (!id.empty())
	{
		LOG(mLog, DEBUG) << "Create input device : " << id;

		if (id[0] == '/')
		{
			return shared_ptr<InputItf::InputDevice<T>>(new DevInput<T>(id));
		}
#ifdef WITH_WAYLAND
		else if (mDisplay)
		{
			return shared_ptr<InputItf::InputDevice<T>>(new WlInput<T>(mDisplay,
																	   id));
		}
#endif
	}

	return shared_ptr<InputItf::InputDevice<T>>();
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

void InputBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
#ifdef WITH_WAYLAND
	addFrontendHandler(FrontendHandlerPtr(
			new InputFrontendHandler(getDeviceName(), getDomId(),
									 domId, devId, mDisplay)));
#else
	addFrontendHandler(FrontendHandlerPtr(
			new InputFrontendHandler(getDeviceName(), getDomId(),
									 domId, devId)));
#endif
}
