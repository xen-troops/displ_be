/*
 * Pointer.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_INPUT_POINTER_HPP_
#define SRC_WAYLAND_INPUT_POINTER_HPP_

#include "wayland/Display.hpp"
#include "InputItf.hpp"

namespace Wayland {

class Pointer : public InputItf::InputDevice<InputItf::PointerCallbacks>
{
public:

	Pointer(Display& display, uint32_t connectorId);
	~Pointer();

	void setCallbacks(const InputItf::PointerCallbacks& callbacks) override;

private:

	XenBackend::Log mLog;

	std::shared_ptr<Surface> mSurface;
	std::shared_ptr<SeatPointer> mSeatPointer;
};

}
#endif /* SRC_WAYLAND_INPUT_POINTER_HPP_ */
