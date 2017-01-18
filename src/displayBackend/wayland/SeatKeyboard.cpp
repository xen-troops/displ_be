/*
 * SeatKeyboard.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#include "SeatKeyboard.hpp"

#include "Exception.hpp"

using std::mutex;
using std::lock_guard;
using std::shared_ptr;

using InputItf::KeyboardCallbacks;

namespace Wayland {

/*******************************************************************************
 * SeatPointer
 ******************************************************************************/

SeatKeyboard::SeatKeyboard(wl_seat* seat) :
	mWlKeyboard(nullptr),
	mLog("SeatKeyboard")
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

SeatKeyboard::~SeatKeyboard()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void SeatKeyboard::sOnKeymap(void* data, wl_keyboard* keyboard, uint32_t format,
							 int32_t fd, uint32_t size)
{

}

void SeatKeyboard::sOnEnter(void* data, wl_keyboard* keyboard, uint32_t serial,
							wl_surface* surface, wl_array* keys)
{
	static_cast<SeatKeyboard*>(data)->onEnter(serial, surface, keys);
}

void SeatKeyboard::sOnLeave(void* data, wl_keyboard* keyboard, uint32_t serial,
							wl_surface* surface)
{
	static_cast<SeatKeyboard*>(data)->onLeave(serial, surface);
}

void SeatKeyboard::sOnKey(void* data, wl_keyboard* keyboard, uint32_t serial,
						  uint32_t time, uint32_t key, uint32_t state)
{
	static_cast<SeatKeyboard*>(data)->onKey(serial, time, key, state);
}

void SeatKeyboard::sOnModifiers(void* data, wl_keyboard* keyboard,
								uint32_t serial, uint32_t modsDepressed,
								uint32_t modsLatched, uint32_t modsLocked,
								uint32_t group)
{

}

void SeatKeyboard::sOnRepeatInfo(void* data, wl_keyboard* keyboard,
								 int32_t rate, int32_t delay)
{

}

void SeatKeyboard::onEnter(uint32_t serial, wl_surface* surface, wl_array* keys)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onEnter surface: " << surface
					  << ", serial: " << serial;

	mCurrentCallback = mCallbacks.find(surface);
}

void SeatKeyboard::onLeave(uint32_t serial, wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onLeave surface: " << surface
					  << ", serial: " << serial;

	mCurrentCallback = mCallbacks.end();
}

void SeatKeyboard::onKey(uint32_t serial, uint32_t time,
						 uint32_t key, uint32_t state)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onKey serial: " << serial << ", time: " << time
					  << ", key: " << key << ", state: " << state;

	if (mCurrentCallback != mCallbacks.end() && mCurrentCallback->second.key)
	{
		mCurrentCallback->second.key(key, state);
	}
}

void SeatKeyboard::init(wl_seat* seat)
{
	mWlKeyboard = wl_seat_get_keyboard(seat);

	if (!mWlKeyboard)
	{
		throw WlException("Can't create keyboard");
	}

	mListener = { sOnKeymap, sOnEnter, sOnLeave, sOnKey,
				  sOnModifiers, sOnRepeatInfo };

	if (wl_keyboard_add_listener(mWlKeyboard, &mListener, this))
	{
		throw WlException("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void SeatKeyboard::release()
{
	if (mWlKeyboard)
	{
		wl_keyboard_destroy(mWlKeyboard);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
