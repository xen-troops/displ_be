/*
 * SeatKeyboard.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SEATKEYBOARD_HPP_
#define SRC_WAYLAND_SEATKEYBOARD_HPP_

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include "SeatDevice.hpp"
#include "InputItf.hpp"

namespace Wayland {

class SeatKeyboard : public SeatDevice<InputItf::KeyboardCallbacks>
{
public:

	~SeatKeyboard();

private:

	friend class Seat;

	SeatKeyboard(wl_seat* seat);

	wl_keyboard* mWlKeyboard;
	wl_keyboard_listener mListener;
	XenBackend::Log mLog;

	static void sOnKeymap(void* data, wl_keyboard* keyboard, uint32_t format,
						  int32_t fd, uint32_t size);
	static void sOnEnter(void* data, wl_keyboard* keyboard, uint32_t serial,
						 wl_surface* surface, wl_array* keys);
	static void sOnLeave(void* data, wl_keyboard* keyboard, uint32_t serial,
						 wl_surface* surface);
	static void sOnKey(void* data, wl_keyboard* keyboard, uint32_t serial,
					   uint32_t time, uint32_t key, uint32_t state);
	static void sOnModifiers(void* data, wl_keyboard* keyboard, uint32_t serial,
							 uint32_t modsDepressed, uint32_t modsLatched,
							 uint32_t modsLocked, uint32_t group);
	static void sOnRepeatInfo(void* data, wl_keyboard* keyboard,
							  int32_t rate, int32_t delay);

	void onEnter(uint32_t serial, wl_surface* surface, wl_array* keys);
	void onLeave(uint32_t serial, wl_surface* surface);
	void onKey(uint32_t serial, uint32_t time, uint32_t key, uint32_t state);

	void init(wl_seat* seat);
	void release();
};

typedef std::shared_ptr<SeatKeyboard> SeatKeyboardPtr;

}

#endif /* SRC_WAYLAND_SEATKEYBOARD_HPP_ */
