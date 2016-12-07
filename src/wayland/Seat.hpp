/*
 * Seat.hpp
 *
 *  Created on: Nov 30, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SEAT_HPP_
#define SRC_WAYLAND_SEAT_HPP_

#include <xen/be/Log.hpp>

#include "Registry.hpp"
#include "ShellSurface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland seat class.
 * @ingroup wayland
 ******************************************************************************/
class Seat : public Registry
{
public:

	~Seat();

private:

	friend class Display;

	Seat(wl_registry* registry, uint32_t id, uint32_t version);

	wl_seat* mWlSeat;
	XenBackend::Log mLog;

	wl_seat_listener mWlListener;

	static void sReadCapabilities(void *data, wl_seat* seat,
								  uint32_t capabilities);
	static void sReadName(void *data, wl_seat* seat, const char *name);

	void readCapabilities(uint32_t capabilities);
	void readName(const std::string& name);

	void init();
	void release();
};

}

#endif /* SRC_WAYLAND_SEAT_HPP_ */
