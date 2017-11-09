/*
 * DevInput.cpp
 *
 *  Created on: Jan 13, 2017
 *      Author: al1
 */

#include <iomanip>

#include <fcntl.h>

#include "DevInput.hpp"

using std::setfill;
using std::setw;
using std::string;
using std::thread;

using XenBackend::PollFd;

using InputItf::Exception;
using InputItf::KeyboardCallbacks;
using InputItf::PointerCallbacks;
using InputItf::TouchCallbacks;

/*******************************************************************************
 * InputBase
 ******************************************************************************/

InputBase::InputBase(const string& name) :
	mLog("InputDevice"),
	mName(name),
	mFd(-1)
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
	stop();
	release();
}

void InputBase::start()
{
	if (!mThread.joinable())
	{
		mThread = thread(&InputBase::run, this);
	}
}

void InputBase::stop()
{
	if (mPollFd)
	{
		mPollFd->stop();
	}

	if (mThread.joinable())
	{
		mThread.join();
	}
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

	LOG(mLog, DEBUG) << "Create: " << mName;
}

void InputBase::release()
{
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
 * InputKeyboard
 ******************************************************************************/

void InputKeyboard::onEvent(const input_event& event)
{
	if (event.type == EV_KEY && mCallbacks.key)
	{
		LOG(mLog, DEBUG) << mName << ", key: " << event.code
						 << ", value: " << event.value;

		mCallbacks.key(event.code, event.value);
	}
}

/*******************************************************************************
 * InputPointer
 ******************************************************************************/

InputPointer::InputPointer(const string& name) : InputDevice(name)
{
	mRelX = mRelY = mRelZ = mAbsX = mAbsY = 0;
	mSendRel = mSendAbs = mSendWheel = false;
}

void InputPointer::onEvent(const input_event& event)
{
	switch(event.type)
	{
		case EV_REL:
			onRelEvent(event);
			break;

		case EV_ABS:
			onAbsEvent(event);
			break;

		case EV_KEY:
			onKeyEvent(event);
			break;

		case EV_SYN:
			onSynEvent(event);
			break;

		default:
			break;
	}
}

void InputPointer::onRelEvent(const input_event& event)
{
	if (event.code == REL_X)
	{
		mRelX = event.value;

		mSendRel = true;
	}
	else if (event.code == REL_Y)
	{
		mRelY = event.value;

		mSendRel = true;
	}
	else if (event.code == REL_WHEEL)
	{
		mRelZ = event.value;

		mSendWheel = true;
	}
}

void InputPointer::onAbsEvent(const input_event& event)
{
	if (event.code == ABS_X)
	{
		mAbsX = event.value;

		mSendAbs = true;
	}
	else if (event.code == ABS_Y)
	{
		mAbsY = event.value;

		mSendAbs = true;
	}
}

void InputPointer::onKeyEvent(const input_event& event)
{
	if (mCallbacks.button)
	{
		LOG(mLog, DEBUG) << mName << ", key: " << event.code
						 << ", value: " << event.value;

		mCallbacks.button(event.code, event.value);
	}
}

void InputPointer::onSynEvent(const input_event& event)
{
	if ((mSendRel || mSendWheel) && mCallbacks.moveRelative)
	{
		LOG(mLog, DEBUG) << mName
						 << ", rel x: " << mRelX
						 << ", rel y: " << mRelY
						 << ", rel z: " << mRelZ;

		mCallbacks.moveRelative(mRelX, mRelY, mRelZ);

		mRelX = mRelY = mRelZ = 0;
		mSendWheel = false;
		mSendRel = false;
	}

	if (mSendAbs && mCallbacks.moveAbsolute)
	{
		LOG(mLog, DEBUG) << mName
						 << ", abs x: " << mAbsX
						 << ", abs y: " << mAbsY
						 << ", rel z: " << mRelZ;

		mCallbacks.moveAbsolute(mAbsX, mAbsY, mRelZ);

		mRelZ = 0;
		mSendWheel = false;
		mSendAbs = false;
	}
}

/*******************************************************************************
 * InputTouch
 ******************************************************************************/

InputTouch::InputTouch(const string& name) : InputDevice(name)
{
	mKey = 0;
	mSendAbs = mSendKey = mMtReport = false;
	mLastMtReports = 0;

	setCurrentSlot(0);
	mContacts[mCurrentSlot].id = -1;

}

void InputTouch::onEvent(const input_event& event)
{
	switch(event.type)
	{
		case EV_ABS:
			onAbsEvent(event);
			break;

		case EV_KEY:
			onKeyEvent(event);
			break;

		case EV_SYN:
			onSynEvent(event);
			break;

		default:
			break;
	}
}

void InputTouch::setCurrentSlot(uint32_t slot)
{
	if (slot >= mContacts.size())
	{
		mContacts.resize(slot + 1, {-1});
	}

	mCurrentSlot = slot;
}

void InputTouch::onAbsEvent(const input_event& event)
{
	switch(event.code)
	{
		case ABS_X:
		case ABS_MT_POSITION_X:
			mContacts[mCurrentSlot].absX = event.value;
			mSendAbs = true;
			break;

		case ABS_Y:
		case ABS_MT_POSITION_Y:
			mContacts[mCurrentSlot].absY = event.value;
			mSendAbs = true;
			break;

		case ABS_MT_SLOT:
			flushEvents();
			setCurrentSlot(event.value);
			break;

		case ABS_MT_TRACKING_ID:
			if (event.value >= 0)
			{
				mContacts[mCurrentSlot].id = event.value;
				mKey = 1;
				mSendKey = true;
			}
			else
			{
				mKey = 0;
				mSendKey = true;

				flushEvents();
			}
			break;
	}
}

void InputTouch::onKeyEvent(const input_event& event)
{
	mKey = event.value;
	mSendKey = true;
}

void InputTouch::onSynEvent(const input_event& event)
{
	if (event.code == SYN_MT_REPORT)
	{
		int32_t nextSlot = mCurrentSlot;

		if (mSendAbs)
		{
			nextSlot = mCurrentSlot + 1;
		}

		if (mContacts[mCurrentSlot].id == -1)
		{
			mKey = 1;
			mSendKey = true;
		}

		flushEvents();

		setCurrentSlot(nextSlot);

		mMtReport = true;
	}

	if (event.code == SYN_REPORT)
	{
		if (mMtReport)
		{
			for(int32_t i = mCurrentSlot; i < mLastMtReports; i++)
			{
				setCurrentSlot(i);
				mKey = 0;
				mSendKey = true;

				flushEvents();
			}

			mLastMtReports = mCurrentSlot;

			setCurrentSlot(0);
			mMtReport = false;
		}

		flushEvents();

		if (mCallbacks.frame)
		{
			LOG(mLog, DEBUG) << mName << ", frame";

			mCallbacks.frame();
		}
	}
}

void InputTouch::flushEvents()
{
	if (mSendKey)
	{
		if ((mKey == 1) && mCallbacks.down)
		{
			LOG(mLog, DEBUG) << mName << ", down"
							 << ", id: " << mCurrentSlot
							 << ", abs x: " << mContacts[mCurrentSlot].absX
							 << ", abs y: " << mContacts[mCurrentSlot].absY;

			mCallbacks.down(mCurrentSlot,
							mContacts[mCurrentSlot].absX,
							mContacts[mCurrentSlot].absY);

			mSendAbs = false;
		}

		if ((mKey == 0) && mCallbacks.up)
		{
			LOG(mLog, DEBUG) << mName << ", up"
							 << ", id: " << mCurrentSlot;

			mCallbacks.up(mCurrentSlot);

			mContacts[mCurrentSlot].id = -1;
		}

		mSendKey = false;
	}

	if (mSendAbs && mCallbacks.motion)
	{
		LOG(mLog, DEBUG) << mName << ", motion"
						 << ", id: " << mCurrentSlot
						 << ", abs x: " << mContacts[mCurrentSlot].absX
						 << ", abs y: " << mContacts[mCurrentSlot].absY;

		mSendAbs = false;

		mCallbacks.motion(mCurrentSlot,
						  mContacts[mCurrentSlot].absX,
						  mContacts[mCurrentSlot].absY);
	}
}
