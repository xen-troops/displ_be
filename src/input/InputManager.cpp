/*
 * InputManager.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: al1
 */

#include "InputManager.hpp"

#include "Exception.hpp"
#include "LibInput.hpp"
#include "WlInput.hpp"

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::to_string;

using Wayland::Display;
using Wayland::Connector;

using InputItf::KeyboardPtr;
using InputItf::PointerPtr;
using InputItf::TouchPtr;

namespace Input {

/*******************************************************************************
 * InputManager
 ******************************************************************************/

InputManager::InputManager(shared_ptr<Display> wlDisplay) :
	InputManager()
{
	mWlDisplay = wlDisplay;
}

InputManager::InputManager() :
	mLog("InputManager")
{
	LOG(mLog, DEBUG) << "Create";
}

InputManager::~InputManager()
{
	mKeyboards.clear();
	mPointers.clear();
	mTouches.clear();

	LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

KeyboardPtr InputManager::createWlKeyboard(int id, uint32_t connectorId)
{
	LOG(mLog, DEBUG) << "Create WL keyboard id: " << id
					 << ", connector: " << connectorId;

	auto surface = dynamic_pointer_cast<Connector>(
			mWlDisplay->getConnectorById(connectorId))->getSurface();

	KeyboardPtr keyboard(
			new WlKeyboard(mWlDisplay->getSeat()->getKeyboard(), surface));

	mKeyboards.emplace(id, keyboard);

	return keyboard;
}

PointerPtr InputManager::createWlPointer(int id, uint32_t connectorId)
{
	LOG(mLog, DEBUG) << "Create WL pointer id: " << id
					 << ", connector: " << connectorId;

	auto surface = dynamic_pointer_cast<Connector>(
			mWlDisplay->getConnectorById(connectorId))->getSurface();

	PointerPtr pointer(
			new WlPointer(mWlDisplay->getSeat()->getPointer(), surface));

	mPointers.emplace(id, pointer);

	return pointer;
}

TouchPtr InputManager::createWlTouch(int id, uint32_t connectorId)
{
	LOG(mLog, DEBUG) << "Create WL touch id: " << id
					 << ", connector: " << connectorId;

	auto surface = dynamic_pointer_cast<Connector>(
			mWlDisplay->getConnectorById(connectorId))->getSurface();

	TouchPtr touch(
			new WlTouch(mWlDisplay->getSeat()->getTouch(), surface));

	mTouches.emplace(id, touch);

	return touch;
}

KeyboardPtr InputManager::createInputKeyboard(int id, const string& name)
{
	LOG(mLog, DEBUG) << "Create Input keyboard id: " << id
					 << ", name: " << name;

	KeyboardPtr keyboard(new InputKeyboard(name));

	mKeyboards.emplace(id, keyboard);

	return keyboard;
}

PointerPtr InputManager::createInputPointer(int id, const string& name)
{
	LOG(mLog, DEBUG) << "Create Input pointer id: " << id
					 << ", name: " << name;

	PointerPtr pointer(new InputPointer(name));

	mPointers.emplace(id, pointer);

	return pointer;
}

TouchPtr InputManager::createInputTouch(int id, const string& name)
{
	LOG(mLog, DEBUG) << "Create Input touch id: " << id
					 << ", name: " << name;

	TouchPtr touch(new InputTouch(name));

	mTouches.emplace(id, touch);

	return touch;
}

KeyboardPtr InputManager::getKeyboard(int id)
{
	auto iter = mKeyboards.find(id);

	if (iter == mKeyboards.end())
	{
		throw Exception("Wrong keyboard id " + to_string(id));
	}

	return iter->second;
}

PointerPtr InputManager::getPointer(int id)
{
	auto iter = mPointers.find(id);

	if (iter == mPointers.end())
	{
		throw Exception("Wrong pointer id " + to_string(id));
	}

	return iter->second;
}

TouchPtr InputManager::getTouch(int id)
{
	auto iter = mTouches.find(id);

	if (iter == mTouches.end())
	{
		throw Exception("Wrong touch id " + to_string(id));
	}

	return iter->second;
}

}

