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

using std::string;
using std::to_string;
using std::vector;

using XenBackend::BackendBase;
using XenBackend::FrontendHandlerPtr;
using XenBackend::Log;
using XenBackend::RingBufferPtr;

using InputItf::InputManagerPtr;

/*******************************************************************************
 * InputFrontendHandler
 ******************************************************************************/

InputFrontendHandler::InputFrontendHandler(
		ConfigPtr config, InputManagerPtr inputManager, const string& devName,
		domid_t beDomId, domid_t feDomId, uint16_t devId) :
	FrontendHandlerBase("VkbdFrontend", devName, beDomId, feDomId, devId),
	mConfig(config),
	mInputManager(inputManager),
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

	InputRingBufferPtr eventRingBuffer(
			new InputRingBuffer(getDomId(), port, ref,
								XENKBD_IN_RING_OFFS, XENKBD_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);

	auto id = getXenStore().readString(getXsFrontendPath() + "/id");

	createKeyboardHandler(eventRingBuffer, id);
	createPointerHandler(eventRingBuffer, id);
	createTouchHandler(eventRingBuffer, id);
}

void InputFrontendHandler::onClosing()
{
	LOG(mLog, DEBUG) << "On frontend closing : " << getDomId();

	mKeyboardHandler.reset();
	mPointerHandler.reset();
	mTouchHandler.reset();
}

void InputFrontendHandler::createKeyboardHandler(InputRingBufferPtr ringBuffer,
												 const string& id)
{
	auto keyboard =  mInputManager->getKeyboard(id);

	if (keyboard)
	{
		mKeyboardHandler.reset( new KeyboardHandler(keyboard, ringBuffer));
	}
}

void InputFrontendHandler::createPointerHandler(InputRingBufferPtr ringBuffer,
												const string& id)
{
	auto pointer =  mInputManager->getPointer(id);

	if (pointer)
	{
		mPointerHandler.reset(new PointerHandler(pointer, ringBuffer));
	}
}

void InputFrontendHandler::createTouchHandler(InputRingBufferPtr ringBuffer,
											  const string& id)
{
	auto touch =  mInputManager->getTouch(id);

	if (touch)
	{
		mTouchHandler.reset(new TouchHandler(touch, ringBuffer));
	}
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

void InputBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
	addFrontendHandler(FrontendHandlerPtr(
			new InputFrontendHandler(mConfig, mInputManager, getDeviceName(),
									 getDomId(), domId, devId)));
}
