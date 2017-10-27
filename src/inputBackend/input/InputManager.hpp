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
	InputItf::KeyboardPtr createWlKeyboard(const std::string& id,
										   const std::string& conName);
	InputItf::PointerPtr createWlPointer(const std::string& id,
										 const std::string& conName);
	InputItf::TouchPtr createWlTouch(const std::string& id,
									 const std::string& conName);
#endif

	InputItf::KeyboardPtr createInputKeyboard(const std::string& id,
											  const std::string& devName);
	InputItf::PointerPtr createInputPointer(const std::string& id,
											const std::string& devName);
	InputItf::TouchPtr createInputTouch(const std::string& id,
										const std::string& devName);

	InputItf::KeyboardPtr getKeyboard(const std::string& id) override;
	InputItf::PointerPtr getPointer(const std::string& id) override;
	InputItf::TouchPtr getTouch(const std::string& id) override;

private:

#ifdef WITH_WAYLAND
	Wayland::DisplayPtr mDisplay;
#endif

	XenBackend::Log mLog;

	std::unordered_map<std::string, InputItf::KeyboardPtr> mKeyboards;
	std::unordered_map<std::string, InputItf::PointerPtr> mPointers;
	std::unordered_map<std::string, InputItf::TouchPtr> mTouches;

};

typedef std::shared_ptr<InputManager> InputManagerPtr;

}

#endif /* SRC_INPUT_INPUTMANAGER_HPP_ */
