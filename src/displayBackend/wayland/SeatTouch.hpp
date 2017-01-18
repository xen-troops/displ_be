/*
 * SeatTouch.hpp
 *
 *  Created on: Jan 5, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SEATTOUCH_HPP_
#define SRC_WAYLAND_SEATTOUCH_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "SeatDevice.hpp"
#include "InputItf.hpp"

namespace Wayland {

class SeatTouch : public SeatDevice<InputItf::TouchCallbacks>
{
public:

	~SeatTouch();

private:

	friend class Seat;

	SeatTouch(wl_seat* seat);

	wl_touch* mWlTouch;
	wl_touch_listener mListener;
	XenBackend::Log mLog;

	static void sOnDown(void* data, wl_touch* touch, uint32_t serial,
						uint32_t time, wl_surface* surface, int32_t id,
						wl_fixed_t x, wl_fixed_t y);
	static void sOnUp(void* data, wl_touch* touch, uint32_t serial,
					  uint32_t time, int32_t id);
	static void sOnMotion(void* data, wl_touch* touch, uint32_t time,
						  int32_t id, wl_fixed_t x, wl_fixed_t y);
	static void sOnFrame(void* data, wl_touch* touch);
	static void sOnCancel(void* data, wl_touch* touch);

	void onDown(uint32_t serial, uint32_t time, wl_surface* surface,
				int32_t id, wl_fixed_t x, wl_fixed_t y);
	void onUp(uint32_t serial, uint32_t time, int32_t id);
	void onMotion(uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
	void onFrame();
	void onCancel();

	void init(wl_seat* seat);
	void release();
};

}

#endif /* SRC_WAYLAND_SEATTOUCH_HPP_ */
