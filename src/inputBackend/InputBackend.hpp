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

#include "Config.hpp"
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
	 * @param inputManager input manager instance
	 * @param backend      backend instance
	 * @param domId        frontend domain id
	 * @param devId        frontend device id
	 */
	InputFrontendHandler(ConfigPtr config,
						 InputItf::InputManagerPtr inputManager,
						 const std::string& devName,
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

	ConfigPtr mConfig;
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
	 * @param inputManager input manager instance
	 * @param deviceName   device name
	 * @param domId        domain id
	 * @param devId        device id
	 */
	InputBackend(ConfigPtr config, InputItf::InputManagerPtr inputManager,
				 const std::string& deviceName) :
		BackendBase("VkbdBackend", deviceName),
		mConfig(config),
		mInputManager(inputManager)
		{}

protected:

	/**
	 * Is called when new input frontend appears.
	 * @param domId domain id
	 * @param devId device id
	 */
	void onNewFrontend(domid_t domId, uint16_t devId);

private:

	ConfigPtr mConfig;
	InputItf::InputManagerPtr mInputManager;
};

#endif /* INPUTBACKEND_HPP_ */
