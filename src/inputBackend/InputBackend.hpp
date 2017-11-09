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

#ifndef INPUTBACKEND_HPP_
#define INPUTBACKEND_HPP_

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/Log.hpp>

#include <xen/io/kbdif.h>

#include "InputItf.hpp"

/***************************************************************************//**
 * @defgroup input_be Input backend
 * Backend related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Ring buffer used to send events to the frontend.
 * @ingroup input_be
 ******************************************************************************/
class InputRingBuffer : public XenBackend::RingBufferOutBase<xenkbd_page,
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
	InputRingBuffer(InputItf::KeyboardPtr keyboard,
					InputItf::PointerPtr pointer,
					InputItf::TouchPtr touch,
					domid_t domId, evtchn_port_t port, int ref,
					int offset, size_t size);

	~InputRingBuffer();

private:

	InputItf::KeyboardPtr mKeyboard;
	InputItf::PointerPtr mPointer;
	InputItf::TouchPtr mTouch;

	XenBackend::Log mLog;

	// keyboard
	void onKey(uint32_t key, uint32_t state);
	// pointer
	void onMoveRel(int32_t x, int32_t y, int32_t z);
	void onMoveAbs(int32_t x, int32_t y, int32_t z);
	void onButton(uint32_t button, uint32_t state);
	// touch
	void onDown(int32_t id, int32_t x, int32_t y);
	void onUp(int32_t id);
	void onMotion(int32_t id, int32_t x, int32_t y);
	void onFrame();
};

typedef std::shared_ptr<InputRingBuffer> InputRingBufferPtr;

/***************************************************************************//**
 * Input frontend handler.
 * @ingroup input_be
 ******************************************************************************/
class InputFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param devName      device name
	 * @param beDomId      backend domain id
	 * @param feDomId      frontend domain id
	 * @param devId        frontend device id
	 */
	InputFrontendHandler(const std::string& devName,
						 domid_t beDomId, domid_t feDomId, uint16_t devId);

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind() override;

	/**
	 * Is called on connected state when ring buffers releases are required.
	 */
	void onClosing() override;

private:

	XenBackend::Log mLog;

	void parseInputId(const std::string& id, std::string& keyboardId,
					  std::string& pointerId, std::string& touchId);

	InputItf::KeyboardPtr createKeyboard(const std::string& id);
	InputItf::PointerPtr createPointer(const std::string& id);
	InputItf::TouchPtr createTouch(const std::string& id);
};

/***************************************************************************//**
 * Input backend class.
 * @ingroup input_be
 ******************************************************************************/
class InputBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param deviceName   device name
	 */
	InputBackend(const std::string& deviceName) :
		BackendBase("VkbdBackend", deviceName)
		{}

protected:

	/**
	 * Is called when new input frontend appears.
	 * @param domId domain id
	 * @param devId device id
	 */
	void onNewFrontend(domid_t domId, uint16_t devId);
};

#endif /* INPUTBACKEND_HPP_ */
