/*
 * SeatTouch.cpp
 *
 *  Created on: Jan 5, 2017
 *      Author: al1
 */

#include "SeatTouch.hpp"

#include "Exception.hpp"

using std::mutex;
using std::lock_guard;
using std::shared_ptr;

using InputItf::PointerCallbacks;

namespace Wayland {

/*******************************************************************************
 * SeatTouch
 ******************************************************************************/

SeatTouch::SeatTouch(wl_seat* seat) :
	mWlTouch(nullptr),
	mLog("SeatTouch")
{
	try
	{
		init(seat);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

SeatTouch::~SeatTouch()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void SeatTouch::sOnDown(void* data, wl_touch* touch, uint32_t serial,
						uint32_t time, wl_surface* surface, int32_t id,
						wl_fixed_t x, wl_fixed_t y)
{
	static_cast<SeatTouch*>(data)->onDown(serial, time, surface, id, x, y);
}

void SeatTouch::sOnUp(void* data, wl_touch* touch, uint32_t serial,
					  uint32_t time, int32_t id)
{
	static_cast<SeatTouch*>(data)->onUp(serial, time, id);
}

void SeatTouch::sOnMotion(void* data, wl_touch* touch, uint32_t time,
						  int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	static_cast<SeatTouch*>(data)->onMotion(time, id, x, y);
}

void SeatTouch::sOnFrame(void* data, wl_touch* touch)
{
	static_cast<SeatTouch*>(data)->onFrame();
}

void SeatTouch::sOnCancel(void* data, wl_touch* touch)
{
	static_cast<SeatTouch*>(data)->onCancel();
}

void SeatTouch::onDown(uint32_t serial, uint32_t time, wl_surface* surface,
					   int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	lock_guard<mutex> lock(mMutex);

	int32_t resX = wl_fixed_to_int(x);
	int32_t resY = wl_fixed_to_int(y);

	DLOG(mLog, DEBUG) << "onDown surface: " << surface
					  << ", serial: " << serial << ", time: " << time
					  << ", id: " << id << ", X: " << resX << ", Y: " << resY;

	mCurrentCallback = mCallbacks.find(surface);

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.down)
	{
		mCurrentCallback->second.down(id, resX, resY);
	}
}

void SeatTouch::onUp(uint32_t serial, uint32_t time, int32_t id)
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onUp serial: " << serial << ", time: " << time
					  << ", id: " << id;

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.up)
	{
		mCurrentCallback->second.up(id);
	}

	mCurrentCallback == mCallbacks.end();
}

void SeatTouch::onMotion(uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	lock_guard<mutex> lock(mMutex);

	int32_t resX = wl_fixed_to_int(x);
	int32_t resY = wl_fixed_to_int(y);

	DLOG(mLog, DEBUG) << "onMotion time: " << time << ", id: " << id
					  << ", X: " << resX << ", Y: " << resY;

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.motion)
	{
		mCurrentCallback->second.motion(id, resX, resY);
	}
}

void SeatTouch::onFrame()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onFrame";

	if (mCurrentCallback != mCallbacks.end() &&
		mCurrentCallback->second.frame)
	{
		mCurrentCallback->second.frame();
	}
}

void SeatTouch::onCancel()
{
	lock_guard<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "onCancel";
}

void SeatTouch::init(wl_seat* seat)
{
	mWlTouch = wl_seat_get_touch(seat);

	if (!mWlTouch)
	{
		throw Exception("Can't create pointer");
	}

	mListener = { sOnDown, sOnUp, sOnMotion, sOnFrame, sOnCancel };

	if (wl_touch_add_listener(mWlTouch, &mListener, this))
	{
		throw Exception("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void SeatTouch::release()
{
	if (mWlTouch)
	{
		wl_touch_destroy(mWlTouch);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
