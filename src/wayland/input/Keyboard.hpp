/*
 * Keyboard.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_INPUT_KEYBOARD_HPP_
#define SRC_WAYLAND_INPUT_KEYBOARD_HPP_

#include "wayland/Display.hpp"
#include "InputItf.hpp"

namespace Wayland {

class Keyboard : public InputItf::InputDevice<InputItf::KeyboardCallbacks>
{
public:

	Keyboard(Display& display, uint32_t connectorId);
	~Keyboard();

	void setCallbacks(const InputItf::KeyboardCallbacks& callbacks) override;

private:

	XenBackend::Log mLog;

	std::shared_ptr<Surface> mSurface;
	std::shared_ptr<SeatKeyboard> mSeatKeyboard;
};

}

#endif /* SRC_WAYLAND_INPUT_KEYBOARD_HPP_ */
