/*
 * Pointer.hpp
 *
 *  Created on: Jan 3, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SEATPOINTER_HPP_
#define SRC_WAYLAND_SEATPOINTER_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "SeatDevice.hpp"
#include "InputItf.hpp"

namespace Wayland {

class SeatPointer : public SeatDevice<InputItf::PointerCallbacks>
{
public:

	~SeatPointer();

private:

	friend class Seat;

	SeatPointer(wl_seat* seat);

	wl_pointer* mWlPointer;
	wl_pointer_listener mListener;
	XenBackend::Log mLog;

	wl_fixed_t mLastX;
	wl_fixed_t mLastY;

	static void sOnEnter(void* data, wl_pointer* pointer, uint32_t serial,
						 wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
	static void sOnLeave(void* data, wl_pointer* pointer, uint32_t serial,
						 wl_surface* surface);
	static void sOnMotion(void* data, wl_pointer* pointer, uint32_t time,
						  wl_fixed_t x, wl_fixed_t y);
	static void sOnButton(void* data, wl_pointer* pointer, uint32_t serial,
						  uint32_t time, uint32_t button, uint32_t state);
	static void sOnAxis(void* data, wl_pointer* pointer, uint32_t time,
						uint32_t axis, wl_fixed_t value);
	static void sOnFrame(void* data, struct wl_pointer* pointer);

	static void sOnAxisSource(void* data, wl_pointer* pointer,
							  uint32_t axisSource);
	static void sOnAxisStop(void* data, wl_pointer* pointer, uint32_t time,
							uint32_t axis);
	static void sOnAxisDiscrete(void* data, wl_pointer* pointer,
								uint32_t axis, int32_t discrete);

	void onEnter(uint32_t serial, wl_surface* surface,
				 wl_fixed_t x, wl_fixed_t y);
	void onLeave(uint32_t serial, wl_surface* surface);
	void onMotion(uint32_t time, wl_fixed_t x, wl_fixed_t y);
	void onButton(uint32_t serial, uint32_t time,
				  uint32_t button, uint32_t state);
	void onAxis(uint32_t time, uint32_t axis, wl_fixed_t value);

	void init(wl_seat* seat);
	void release();
};

}

#endif /* SRC_WAYLAND_SEATPOINTER_HPP_ */
