/*
 * SeatPointer.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#include "SeatPointer.hpp"

#include "Exception.hpp"

using std::mutex;
using std::lock_guard;
using std::shared_ptr;

using InputItf::PointerCallbacks;

namespace Wayland {

/*******************************************************************************
 * SeatPointer
 ******************************************************************************/

SeatPointer::SeatPointer(wl_seat* seat) :
	mWlPointer(nullptr),
	mLog("SeatPointer")
{
	try
	{
		init(seat);
	}
	catch(const WlException& e)
	{
		release();

		throw;
	}
}

SeatPointer::~SeatPointer()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void SeatPointer::sOnEnter(void* data, wl_pointer* pointer, uint32_t serial,
						   wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
	static_cast<SeatPointer*>(data)->onEnter(serial, surface, x, y);
}

void SeatPointer::sOnLeave(void* data, wl_pointer* pointer, uint32_t serial,
						   wl_surface* surface)
{
	static_cast<SeatPointer*>(data)->onLeave(serial, surface);
}

void SeatPointer::sOnMotion(void* data, wl_pointer* pointer, uint32_t time,
							wl_fixed_t x, wl_fixed_t y)
{
	static_cast<SeatPointer*>(data)->onMotion(time, x, y);
}

void SeatPointer::sOnButton(void* data, wl_pointer* pointer, uint32_t serial,
							uint32_t time, uint32_t button, uint32_t state)
{
	static_cast<SeatPointer*>(data)->onButton(serial, time, button, state);
}

void SeatPointer::sOnAxis(void* data, wl_pointer* pointer, uint32_t time,
						  uint32_t axis, wl_fixed_t value)
{
	static_cast<SeatPointer*>(data)->onAxis(time, axis, value);
}

void SeatPointer::sOnFrame(void* data, struct wl_pointer* pointer)
{

}

void SeatPointer::sOnAxisSource(void* data, wl_pointer* pointer,
								uint32_t axisSource)
{

}

void SeatPointer::sOnAxisStop(void* data, wl_pointer* pointer, uint32_t time,
							  uint32_t axis)
{

}

void SeatPointer::sOnAxisDiscrete(void* data, wl_pointer* pointer,
								  uint32_t axis, int32_t discrete)
{

}

void SeatPointer::onEnter(uint32_t serial, wl_surface* surface,
						  wl_fixed_t x, wl_fixed_t y)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onEnter serial: " << serial
					  << ", X: " << x << ", Y: " << y;

	mCurrentCallback = mCallbacks.find(surface);

	mLastX = x;
	mLastY = y;
}

void SeatPointer::onLeave(uint32_t serial, wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onLeave serial: " << serial;

	mCurrentCallback = mCallbacks.end();
}

void SeatPointer::onMotion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onMotion time: " << time
					  << ", X: " << x << ", Y: " << y;

	if (mCurrentCallback != mCallbacks.end())
	{
		if (mCurrentCallback->second.moveRelative)
		{
			mCurrentCallback->second.moveRelative(x - mLastX, y - mLastY);
		}

		if (mCurrentCallback->second.moveAbsolute)
		{
			mCurrentCallback->second.moveAbsolute(x, y);
		}
	}

	mLastX = x;
	mLastY = y;
}

void SeatPointer::onButton(uint32_t serial, uint32_t time,
						   uint32_t button, uint32_t state)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onButton serial: " << serial << ", time: " << time
					  << ", button: " << button << ", state: " << state;

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.button)
	{
		mCurrentCallback->second.button(button, state);
	}
}

void SeatPointer::onAxis(uint32_t time, uint32_t axis, wl_fixed_t value)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onAxis time: " << time << ", axis: " << axis
					  << ", value: " << value;

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.axis)
	{
		mCurrentCallback->second.axis(axis, value);
	}
}

void SeatPointer::init(wl_seat* seat)
{
	mWlPointer = wl_seat_get_pointer(seat);

	if (!mWlPointer)
	{
		throw WlException("Can't create pointer");
	}

	// Wayland 1.11
	// mListener = { sOnEnter, sOnLeave, sOnMotion, sOnButton, sOnAxis, sOnFrame,
	//			  sOnAxisSource, sOnAxisStop, sOnAxisDiscrete };
	//

	mListener = { sOnEnter, sOnLeave, sOnMotion, sOnButton, sOnAxis };

	if (wl_pointer_add_listener(mWlPointer, &mListener, this))
	{
		throw WlException("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void SeatPointer::release()
{
	if (mWlPointer)
	{
		wl_pointer_destroy(mWlPointer);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
