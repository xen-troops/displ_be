/*
 * WlKeyboard.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_WLINPUT_HPP_
#define SRC_INPUT_WLINPUT_HPP_

#include "wayland/Display.hpp"
#include "InputItf.hpp"

template <typename T>
class WlInput : public InputItf::InputDevice<T>
{
public:

	WlInput(Wayland::DisplayPtr display, const std::string& connector) :
		mDisplay(display),
		mConnector(connector)
	{
	}

	~WlInput()
	{
		mDisplay->clearInputCallbacks<T>(mConnector);
	}

	void setCallbacks(const T& callbacks) override
	{
		mDisplay->setInputCallbacks(mConnector, callbacks);
	}

private:

	Wayland::DisplayPtr mDisplay;
	std::string mConnector;

};

typedef WlInput<InputItf::KeyboardCallbacks> WlKeyboard;
typedef WlInput<InputItf::PointerCallbacks> WlPointer;
typedef WlInput<InputItf::TouchCallbacks> WlTouch;

#endif /* SRC_INPUT_WLINPUT_HPP_ */
