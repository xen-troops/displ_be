/*
 * LibInput.cpp
 *
 *  Created on: Jan 13, 2017
 *      Author: al1
 */

#include "LibInput.hpp"

#include <iomanip>

#include <fcntl.h>

#include "Exception.hpp"

using std::setfill;
using std::setw;
using std::string;
using std::thread;

using XenBackend::PollFd;

using InputItf::KeyboardCallbacks;
using InputItf::PointerCallbacks;
using InputItf::TouchCallbacks;

namespace Input {

/*******************************************************************************
 * InputBase
 ******************************************************************************/

InputBase::InputBase(const string& name) :
	mName(name),
	mFd(-1),
	mLog("InputDevice")
{
	try
	{
		init();
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

InputBase::~InputBase()
{
	release();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void InputBase::init()
{
	mFd = open(mName.c_str(), O_RDONLY);

	if (mFd < 0)
	{
		throw Exception(strerror(errno));
	}

	if (ioctl(mFd, EVIOCGRAB, reinterpret_cast<void*>(1)))
	{
		throw Exception("Grabbed by another process");
	}

	ioctl(mFd, EVIOCGRAB, reinterpret_cast<void*>(0));

	mPollFd.reset(new PollFd(mFd, POLLIN));

	mThread = thread(&InputBase::run, this);

	LOG(mLog, DEBUG) << "Create: " << mName;
}

void InputBase::release()
{
	mPollFd->stop();

	if (mThread.joinable())
	{
		mThread.join();
	}

	if (mFd >= 0)
	{
		close(mFd);

		LOG(mLog, DEBUG) << "Delete: " << mName;
	}
}

void InputBase::run()
{
	try
	{
		while(mPollFd->poll())
		{
			int readSize = 0;

			input_event events[64];

			readSize = read(mFd, events, sizeof(events));

			if (readSize < static_cast<int>(sizeof(struct input_event)))
			{
				throw Exception("Read error");
			}

			for(size_t i = 0; i < readSize/sizeof(input_event); i++)
			{
				onEvent(events[i]);
			}
		}
	}
	catch(const std::exception& e)
	{
		LOG(mLog, ERROR) << e.what();
	}
}

/*******************************************************************************
 * InputDevice
 ******************************************************************************/

template <>
void InputDevice<KeyboardCallbacks>::onEvent(const input_event& event)
{

}

template <>
void InputDevice<PointerCallbacks>::onEvent(const input_event& event)
{

}

template <>
void InputDevice<TouchCallbacks>::onEvent(const input_event& event)
{

}

}
