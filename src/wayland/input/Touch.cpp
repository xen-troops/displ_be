/*
 * Touch.cpp
 *
 *  Created on: Jan 5, 2017
 *      Author: al1
 */

#include "Touch.hpp"

using std::dynamic_pointer_cast;

using  InputItf::TouchCallbacks;

namespace Wayland {

/*******************************************************************************
 * Touch
 ******************************************************************************/

Touch::Touch(Display& display, uint32_t connectorId) :
	mLog("Touch")
{
	mSeatTouch = display.getSeat()->getTouch();

	if (!mSeatTouch)
	{
		throw InputItf::Exception("No seat touch");
	}

	mSurface = dynamic_pointer_cast<Connector>(
		display.getConnectorById(connectorId))->getSurface();

	LOG(mLog, DEBUG) << "Create";
}

Touch::~Touch()
{
	mSeatTouch->clearCallbacks(mSurface);

	LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Touch::setCallbacks(const TouchCallbacks& callbacks)
{
	LOG(mLog, DEBUG) << "setCallbacks";

	mSeatTouch->setCallbacks(mSurface, callbacks);
}

}
