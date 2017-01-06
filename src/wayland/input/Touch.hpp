/*
 * Touch.hpp
 *
 *  Created on: Jan 5, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_INPUT_TOUCH_HPP_
#define SRC_WAYLAND_INPUT_TOUCH_HPP_

#include "wayland/Display.hpp"
#include "InputItf.hpp"

namespace Wayland {

class Touch : public InputItf::InputDevice<InputItf::TouchCallbacks>
{
public:

	Touch(Display& display, uint32_t connectorId);
	~Touch();

	void setCallbacks(const InputItf::TouchCallbacks& callbacks) override;

private:

	XenBackend::Log mLog;

	std::shared_ptr<Surface> mSurface;
	std::shared_ptr<SeatTouch> mSeatTouch;
};

}

#endif /* SRC_WAYLAND_INPUT_TOUCH_HPP_ */
