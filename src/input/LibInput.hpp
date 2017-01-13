/*
 * LibInput.hpp
 *
 *  Created on: Jan 13, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_LIBINPUT_HPP_
#define SRC_INPUT_LIBINPUT_HPP_

#include <string>
#include <thread>

#include <linux/input.h>

#include <xen/be/Log.hpp>

#include "InputItf.hpp"

namespace Input {

class InputBase
{
public:

	InputBase(const std::string& name);
	virtual ~InputBase();

protected:

	virtual void onEvent(const input_event& event) = 0;

private:

	const int cPoolEventTimeoutMs = 100;

	std::string mName;
	int mFd;
	XenBackend::Log mLog;

	std::atomic_bool mTerminate;

	std::thread mThread;

	void init();
	void release();

	void run();
};

template <class T>
class InputDevice : public InputBase, public InputItf::InputDevice<T>
{
public:

	InputDevice(const std::string& name) : InputBase(name) {}

	void setCallbacks(const T& callbacks) override
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mCallbacks = callbacks;
	}

	void onEvent(const input_event& event) override;

private:

	T mCallbacks;
	std::mutex mMutex;
};

#if 0
template <>
void InputDevice<InputItf::KeyboardCallbacks>::onEvent(
		const input_event& event);
#endif

typedef InputDevice<InputItf::KeyboardCallbacks> InputKeyboard;
typedef InputDevice<InputItf::PointerCallbacks> InputPointer;
typedef InputDevice<InputItf::TouchCallbacks> InputTouch;

}

#endif /* SRC_INPUT_LIBINPUT_HPP_ */
