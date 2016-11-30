/*
 * Seat.cpp
 *
 *  Created on: Nov 30, 2016
 *      Author: al1
 */

#include "Seat.hpp"

#include "Exception.hpp"

using std::shared_ptr;
using std::string;

namespace Wayland {

/*******************************************************************************
 * Seat
 ******************************************************************************/

Seat::Seat(wl_registry* registry, uint32_t id, uint32_t version) :
	Registry(registry, id, version),
	mSeat(nullptr),
	mLog("Seat")
{
	try
	{
		init();
	}
	catch(const WlException& e)
	{
		release();

		throw;
	}
}

Seat::~Seat()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void Seat::sReadCapabilities(void *data, wl_seat* seat,
							 uint32_t capabilities)
{
	static_cast<Seat*>(data)->readCapabilities(capabilities);
}

void Seat::sReadName(void *data, wl_seat* seat, const char *name)
{
	static_cast<Seat*>(data)->readName(string(name));
}

void Seat::readCapabilities(uint32_t capabilities)
{
	if (capabilities & WL_SEAT_CAPABILITY_POINTER)
	{
		LOG(mLog, DEBUG) << "Display has a pointer";
	}

	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
	{
		LOG(mLog, DEBUG) << "Display has a keyboard";
	}

	if (capabilities & WL_SEAT_CAPABILITY_TOUCH)
	{
		LOG(mLog, DEBUG) << "Display has a touch screen";
	}
}

void Seat::readName(const std::string& name)
{
	LOG(mLog, DEBUG) << "Name: " << name;
}

void Seat::init()
{
	mSeat = static_cast<wl_seat*>(
			wl_registry_bind(getRegistry(), getId(),
							 &wl_seat_interface, getVersion()));

	if (!mSeat)
	{
		throw WlException("Can't bind seat");
	}

	mListener = {sReadCapabilities, sReadName};

	if (wl_seat_add_listener(mSeat, &mListener, this) < 0)
	{
		throw WlException("Can't add listener");
	}

	LOG(mLog, DEBUG) << "Create";
}

void Seat::release()
{
	if (mSeat)
	{
		wl_seat_destroy(mSeat);

		LOG(mLog, DEBUG) << "Delete";
	}
}

}
