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

using Wayland::Display;

using InputItf::InputManagerPtr;

/*******************************************************************************
 * InputFrontendHandler
 ******************************************************************************/

InputFrontendHandler::InputFrontendHandler(
		InputManagerPtr inputManager, BackendBase& backend,
		domid_t domId, uint16_t devId) :
	FrontendHandlerBase("VkbdFrontend", backend, domId, devId),
	mInputManager(inputManager),
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

	InputRingBufferPtr eventRingBuffer(
			new InputRingBuffer(getDomId(), port, ref,
								XENKBD_IN_RING_OFFS, XENKBD_IN_RING_SIZE));

	addRingBuffer(eventRingBuffer);

	createKeyboardHandler(eventRingBuffer);
	createPointerHandler(eventRingBuffer);
	createTouchHandler(eventRingBuffer);
}

void InputFrontendHandler::createKeyboardHandler(InputRingBufferPtr ringBuffer)
{
	try
	{
		mKeyboardHandler.reset(
			new KeyboardHandler(mInputManager->getKeyboard(getDevId()),
														   ringBuffer));
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

void InputFrontendHandler::createPointerHandler(InputRingBufferPtr ringBuffer)
{
	try
	{
		mPointerHandler.reset(
			new PointerHandler(mInputManager->getPointer(getDevId()),
														 ringBuffer));
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

void InputFrontendHandler::createTouchHandler(InputRingBufferPtr ringBuffer)
{
	try
	{
		mTouchHandler.reset(
			new TouchHandler(mInputManager->getTouch(getDevId()), ringBuffer));
	}
	catch(const InputItf::Exception& e)
	{
		LOG(mLog, WARNING) << e.what();
	}
}

/*******************************************************************************
 * DisplayBackend
 ******************************************************************************/

void InputBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
	addFrontendHandler(FrontendHandlerPtr(
			new InputFrontendHandler(mInputManager, *this, domId, devId)));
}
