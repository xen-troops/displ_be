/*
 * WlKeyboard.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_WLINPUT_HPP_
#define SRC_INPUT_WLINPUT_HPP_

#include "InputItf.hpp"

#include "wayland/SeatKeyboard.hpp"
#include "wayland/SeatPointer.hpp"
#include "wayland/SeatTouch.hpp"
#include "wayland/Surface.hpp"

namespace Input {

template <class S, class T>
class WlInput : public InputItf::InputDevice<T>
{
public:

	WlInput(S seat, std::shared_ptr<Wayland::Surface> surface) :
		mSeat(seat),
		mSurface(surface)
	{
		if (!mSeat)
		{
			throw InputItf::Exception("No seat for device");
		}
	}

	~WlInput()
	{
		mSeat->clearCallbacks(mSurface);
	}

	void setCallbacks(const T& callbacks) override
	{
		mSeat->setCallbacks(mSurface, callbacks);
	}

private:

	S mSeat;
	std::shared_ptr<Wayland::Surface> mSurface;
};

typedef WlInput<std::shared_ptr<Wayland::SeatKeyboard>,
				InputItf::KeyboardCallbacks> WlKeyboard;

typedef WlInput<std::shared_ptr<Wayland::SeatPointer>,
				InputItf::PointerCallbacks> WlPointer;

typedef WlInput<std::shared_ptr<Wayland::SeatTouch>,
				InputItf::TouchCallbacks> WlTouch;

}

#endif /* SRC_INPUT_WLINPUT_HPP_ */
