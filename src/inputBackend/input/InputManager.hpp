/*
 * InputManager.hpp
 *
 *  Created on: Jan 12, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_INPUTMANAGER_HPP_
#define SRC_INPUT_INPUTMANAGER_HPP_

#include <unordered_map>

#include "wayland/Display.hpp"
#include "InputItf.hpp"

namespace Input {

class InputManager : public InputItf::InputManager
{
public:

	InputManager();
	InputManager(Wayland::DisplayPtr wlDisplay);
	~InputManager();

	InputItf::KeyboardPtr createWlKeyboard(int id, uint32_t connectorId);
	InputItf::PointerPtr createWlPointer(int id, uint32_t connectorId);
	InputItf::TouchPtr createWlTouch(int id, uint32_t connectorId);

	InputItf::KeyboardPtr createInputKeyboard(int id, const std::string& name);
	InputItf::PointerPtr createInputPointer(int id, const std::string& name);
	InputItf::TouchPtr createInputTouch(int id, const std::string& name);

	InputItf::KeyboardPtr getKeyboard(int id) override;
	InputItf::PointerPtr getPointer(int id) override;
	InputItf::TouchPtr getTouch(int id) override;

private:

	Wayland::DisplayPtr mDisplay;

	XenBackend::Log mLog;

	std::unordered_map<int, InputItf::KeyboardPtr> mKeyboards;
	std::unordered_map<int, InputItf::PointerPtr> mPointers;
	std::unordered_map<int, InputItf::TouchPtr> mTouches;

};

typedef std::shared_ptr<InputManager> InputManagerPtr;

}

#endif /* SRC_INPUT_INPUTMANAGER_HPP_ */
