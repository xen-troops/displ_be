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

#include <vector>

#include "InputHandlers.hpp"
#include "wayland/input/Keyboard.hpp"
#include "wayland/input/Pointer.hpp"
#include "wayland/input/Touch.hpp"

using std::shared_ptr;
using std::string;
using std::to_string;
using std::vector;

using XenBackend::BackendBase;
using XenBackend::FrontendHandlerPtr;
using XenBackend::Log;
using XenBackend::RingBufferPtr;

using Wayland::Display;
using Wayland::Keyboard;
using Wayland::Pointer;
using Wayland::Touch;

using InputItf::KeyboardPtr;
using InputItf::PointerPtr;
using InputItf::TouchPtr;

/*******************************************************************************
 * InputFrontendHandler
 ******************************************************************************/

InputFrontendHandler::InputFrontendHandler(shared_ptr<Display> display,
										   BackendBase& backend,
										   domid_t domId, int id) :
	FrontendHandlerBase("VkbdFrontend", backend, domId, id),
	mDisplay(display),
	mLog("InputFrontend")
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

	EventRingBufferPtr eventRingBuffer(
			new EventRingBuffer(getDomId(), port, ref,
								XENKBD_IN_RING_OFFS, XENKBD_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);

	createKeyboardHandler(eventRingBuffer);
	createPointerHandler(eventRingBuffer);

	createTouchHandlers();
}

void InputFrontendHandler::createKeyboardHandler(EventRingBufferPtr ringBuffer)
{
	try
	{
		KeyboardPtr keyboardDevice(new Keyboard(*mDisplay.get(), 0));

		mKeyboardHandler.reset(new KeyboardHandler(keyboardDevice, ringBuffer));
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

void InputFrontendHandler::createPointerHandler(EventRingBufferPtr ringBuffer)
{
	try
	{
		PointerPtr pointerDevice(new Pointer(*mDisplay.get(), 0));

		mPointerHandler.reset(new PointerHandler(pointerDevice, ringBuffer));
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

void InputFrontendHandler::createTouchHandlers()
{
	static int sConnectorId = 0;

	try
	{
		const vector<string> touchPathes = getXenStore().readDirectory(
				getXsFrontendPath() + "/" XENKBD_PATH_MTOUCH);

		for(auto touchPath : touchPathes)
		{
			string path = getXsFrontendPath() + "/" XENKBD_PATH_MTOUCH "/" +
						  touchPath;

			evtchn_port_t port = getXenStore().readUint(
					path + "/" XENKBD_FIELD_EVT_CHANNEL);
			grant_ref_t ref = getXenStore().readUint(
					path + "/" XENKBD_FIELD_RING_GREF);

			EventRingBufferPtr ringBuffer(
					new EventRingBuffer(getDomId(), port, ref,
										XENKBD_IN_RING_OFFS,
										XENKBD_IN_RING_SIZE));

			addRingBuffer(ringBuffer);

			TouchPtr touchDevice(new Touch(*mDisplay.get(), sConnectorId++));

			mTouchHandlers.emplace_back(new TouchHandler(touchDevice,
														 ringBuffer));
		}
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

void InputBackend::onNewFrontend(domid_t domId, int id)
{
	addFrontendHandler(FrontendHandlerPtr(
			new InputFrontendHandler(mDisplay, *this, domId, id)));
}
