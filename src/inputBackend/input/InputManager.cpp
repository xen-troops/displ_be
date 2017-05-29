/*
 * InputManager.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: al1
 */

#include "InputManager.hpp"

#include "DevInput.hpp"
#include "Exception.hpp"
#ifdef WITH_WAYLAND
#include "WlInput.hpp"
#endif

using std::dynamic_pointer_cast;
using std::string;
using std::to_string;

#ifdef WITH_WAYLAND
using Wayland::Connector;
using Wayland::DisplayPtr;
#endif

using InputItf::KeyboardPtr;
using InputItf::PointerPtr;
using InputItf::TouchPtr;

namespace Input {

/*******************************************************************************
 * InputManager
 ******************************************************************************/

#ifdef WITH_WAYLAND
InputManager::InputManager(DisplayPtr wlDisplay) :
	InputManager()
{
	mDisplay = wlDisplay;
}
#endif

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

#ifdef WITH_WAYLAND
KeyboardPtr InputManager::createWlKeyboard(int id, const string& conName)
{
	LOG(mLog, DEBUG) << "Create WL keyboard id: " << id
					 << ", connector: " << conName;

	if (!mDisplay)
	{
		throw Exception("Display is not set.");
	}

	auto surface = dynamic_pointer_cast<Connector>(
			mDisplay->getConnectorByName(conName))->getSurface();

	KeyboardPtr keyboard(
			new WlKeyboard(mDisplay->getSeat()->getKeyboard(), surface));

	mKeyboards.emplace(id, keyboard);

	return keyboard;
}

PointerPtr InputManager::createWlPointer(int id, const string& conName)
{
	LOG(mLog, DEBUG) << "Create WL pointer id: " << id
					 << ", connector: " << conName;

	if (!mDisplay)
	{
		throw Exception("Display is not set.");
	}

	auto surface = dynamic_pointer_cast<Connector>(
			mDisplay->getConnectorByName(conName))->getSurface();

	PointerPtr pointer(
			new WlPointer(mDisplay->getSeat()->getPointer(), surface));

	mPointers.emplace(id, pointer);

	return pointer;
}

TouchPtr InputManager::createWlTouch(int id, const string& conName)
{
	LOG(mLog, DEBUG) << "Create WL touch id: " << id
					 << ", connector: " << conName;

	if (!mDisplay)
	{
		throw Exception("Display is not set.");
	}

	auto surface = dynamic_pointer_cast<Connector>(
			mDisplay->getConnectorByName(conName))->getSurface();

	TouchPtr touch(
			new WlTouch(mDisplay->getSeat()->getTouch(), surface));

	mTouches.emplace(id, touch);

	return touch;
}
#endif

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
