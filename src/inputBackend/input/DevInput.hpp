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

class DevInputBase
{
public:

	DevInputBase(const std::string& name);
	virtual ~DevInputBase();

	void start();
	void stop();

protected:

	XenBackend::Log mLog;
	std::string mName;
	int mFd;

	virtual void onEvent(const input_event& event) = 0;

private:

	const int cPoolEventTimeoutMs = 100;

	std::thread mThread;

	std::unique_ptr<XenBackend::PollFd> mPollFd;

	void init();
	void release();

	void run();
};

template <typename T>
class DevInputCbk : public DevInputBase, public InputItf::InputDevice<T>
{
public:

	DevInputCbk(const std::string& name) : DevInputBase(name) {}

	void setCallbacks(const T& callbacks) override
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mCallbacks = callbacks;
	}

protected:

	T mCallbacks;

	std::mutex mMutex;
};

template <typename T>
class DevInput : public DevInputCbk<T>
{
public:

	using DevInputCbk<T>::DevInputCbk;

	void onEvent(const input_event& event) override;
};

template <>
class DevInput<InputItf::KeyboardCallbacks> :
	public DevInputCbk<InputItf::KeyboardCallbacks>
{
public:

	DevInput(const std::string& name);
	~DevInput();

	void onEvent(const input_event& event) override;
};

template<>
class DevInput<InputItf::PointerCallbacks> :
	public DevInputCbk<InputItf::PointerCallbacks>
{
public:

	DevInput(const std::string& name, uint32_t widht, uint32_t height);
	~DevInput();

	void onEvent(const input_event& event) override;
	void setCallbacks(const InputItf::PointerCallbacks& callbacks) override;

private:

	int32_t mRelX, mRelY, mRelZ, mAbsX, mAbsY;
	bool mSendRel, mSendAbs, mSendWheel;
	uint32_t mWidth, mHeight;

	void onRelEvent(const input_event& event);
	void onAbsEvent(const input_event& event);
	void onKeyEvent(const input_event& event);
	void onSynEvent(const input_event& event);
};



template<>
class DevInput<InputItf::TouchCallbacks>:
	public DevInputCbk<InputItf::TouchCallbacks>
{
public:

	DevInput(const std::string& name);
	~DevInput();

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
