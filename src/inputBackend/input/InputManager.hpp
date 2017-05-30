/*
 * InputManager.hpp
 *
 *  Created on: Jan 12, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_INPUTMANAGER_HPP_
#define SRC_INPUT_INPUTMANAGER_HPP_

#include <unordered_map>

#include <xen/be/Log.hpp>

#ifdef WITH_WAYLAND
#include "wayland/Display.hpp"
#endif

#include "InputItf.hpp"

namespace Input {

class InputManager : public InputItf::InputManager
{
public:

	InputManager();

#ifdef WITH_WAYLAND
	InputManager(Wayland::DisplayPtr wlDisplay);
#endif

	~InputManager();

#ifdef WITH_WAYLAND
	InputItf::KeyboardPtr createWlKeyboard(int id, const std::string& conName);
	InputItf::PointerPtr createWlPointer(int id, const std::string& conName);
	InputItf::TouchPtr createWlTouch(int id, const std::string& conName);
#endif

	InputItf::KeyboardPtr createInputKeyboard(
			int id, const std::string& devName);
	InputItf::PointerPtr createInputPointer(int id, const std::string& devName);
	InputItf::TouchPtr createInputTouch(int id, const std::string& devName);

	InputItf::KeyboardPtr getKeyboard(int id) override;
	InputItf::PointerPtr getPointer(int id) override;
	InputItf::TouchPtr getTouch(int id) override;

private:

#ifdef WITH_WAYLAND
	Wayland::DisplayPtr mDisplay;
#endif

	XenBackend::Log mLog;

	std::unordered_map<int, InputItf::KeyboardPtr> mKeyboards;
	std::unordered_map<int, InputItf::PointerPtr> mPointers;
	std::unordered_map<int, InputItf::TouchPtr> mTouches;

};

typedef std::shared_ptr<InputManager> InputManagerPtr;

}

#endif /* SRC_INPUT_INPUTMANAGER_HPP_ */
