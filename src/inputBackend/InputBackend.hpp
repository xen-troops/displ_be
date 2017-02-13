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

#include "DisplayItf.hpp"
#include "wayland/Display.hpp"
#include "InputHandlers.hpp"

/***************************************************************************//**
 * @defgroup input_be Input backend
 * Backend related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Input frontend handler.
 * @ingroup input_be
 ******************************************************************************/
class InputFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param domId     frontend domain id
	 * @param backend   backend instance
	 * @param id        frontend instance id
	 */
	InputFrontendHandler(InputItf::InputManagerPtr inputManager,
						 XenBackend::BackendBase& backend,
						 domid_t domId, int id);

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind();

private:

	InputItf::InputManagerPtr mInputManager;
	XenBackend::Log mLog;

	std::unique_ptr<KeyboardHandler> mKeyboardHandler;
	std::unique_ptr<PointerHandler> mPointerHandler;
	std::unique_ptr<TouchHandler> mTouchHandler;

	void createKeyboardHandler(InputRingBufferPtr ringBuffer);
	void createPointerHandler(InputRingBufferPtr ringBuffer);
	void createTouchHandler(InputRingBufferPtr ringBuffer);
};

/***************************************************************************//**
 * Input backend class.
 * @ingroup input_be
 ******************************************************************************/
class InputBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param domId         domain id
	 * @param deviceName    device name
	 * @param id            instance id
	 */
	InputBackend(InputItf::InputManagerPtr inputManager,
				 const std::string& deviceName, domid_t domId, int id) :
		BackendBase("VkbdBackend", deviceName, domId, id),
		mInputManager(inputManager)
		{}

protected:

	/**
	 * Is called when new input frontend appears.
	 * @param domId domain id
	 * @param id    instance id
	 */
	void onNewFrontend(domid_t domId, int id);

private:

	InputItf::InputManagerPtr mInputManager;
};

#endif /* INPUTBACKEND_HPP_ */
