/*
 * Pointer.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#include "Pointer.hpp"

using std::dynamic_pointer_cast;

using  InputItf::PointerCallbacks;

namespace Wayland {

/*******************************************************************************
 * Pointer
 ******************************************************************************/

Pointer::Pointer(Display& display, uint32_t connectorId)
{
	mSeatPointer = display.getSeat()->getPointer();

	if (!mSeatPointer)
	{
		throw InputItf::Exception("No seat pointer");
	}

	mSurface = dynamic_pointer_cast<Connector>(
		display.getConnectorById(connectorId))->getSurface();
}

Pointer::~Pointer()
{
	mSeatPointer->clearCallbacks(mSurface);
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Pointer::setCallbacks(const PointerCallbacks& callbacks)
{
	mSeatPointer->setCallbacks(mSurface, callbacks);
}

}
