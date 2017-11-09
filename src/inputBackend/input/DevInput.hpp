/*
 * DevInput.hpp
 *
 *  Created on: Jan 13, 2017
 *      Author: al1
 */

#ifndef SRC_INPUT_DEVINPUT_HPP_
#define SRC_INPUT_DEVINPUT_HPP_

#include <string>
#include <thread>

#include <linux/input.h>

#include <xen/be/Log.hpp>
#include <xen/be/Utils.hpp>

#include "InputItf.hpp"

// TODO: should be reimplemented to use libinput

class InputBase
{
public:

	InputBase(const std::string& name);
	virtual ~InputBase();

	void start();
	void stop();

protected:

	XenBackend::Log mLog;
	std::string mName;

	virtual void onEvent(const input_event& event) = 0;

private:

	const int cPoolEventTimeoutMs = 100;

	int mFd;

	std::thread mThread;

	std::unique_ptr<XenBackend::PollFd> mPollFd;

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

	void start() override { InputBase::start(); }
	void stop() override { InputBase::stop(); }

protected:

	T mCallbacks;

private:

	std::mutex mMutex;
};

class InputKeyboard : public InputDevice<InputItf::KeyboardCallbacks>
{
public:

	using InputDevice<InputItf::KeyboardCallbacks>::InputDevice;

	void onEvent(const input_event& event) override;
};

class InputPointer : public InputDevice<InputItf::PointerCallbacks>
{
public:

	InputPointer(const std::string& name);

	void onEvent(const input_event& event) override;

private:

	int32_t mRelX, mRelY, mRelZ, mAbsX, mAbsY;
	bool mSendRel, mSendAbs, mSendWheel;

	void onRelEvent(const input_event& event);
	void onAbsEvent(const input_event& event);
	void onKeyEvent(const input_event& event);
	void onSynEvent(const input_event& event);
};

class InputTouch: public InputDevice<InputItf::TouchCallbacks>
{
public:

	InputTouch(const std::string& name);

	void onEvent(const input_event& event) override;

private:

	struct Contact
	{
		int32_t id;
		int32_t absX;
		int32_t absY;
	};

	int32_t mKey;
	bool mSendAbs, mSendKey;
	bool mMtReport;
	int32_t mLastMtReports;

	uint32_t mCurrentSlot;
	std::vector<Contact> mContacts;

	void setCurrentSlot(uint32_t slot);
	void onAbsEvent(const input_event& event);
	void onKeyEvent(const input_event& event);
	void onSynEvent(const input_event& event);
	void flushEvents();
};

#endif /* SRC_INPUT_DEVINPUT_HPP_ */
