/*
 * Keyboard.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#include "Keyboard.hpp"

using std::dynamic_pointer_cast;

using InputItf::KeyboardCallbacks;

namespace Wayland {

/*******************************************************************************
 * Pointer
 ******************************************************************************/

Keyboard::Keyboard(Display& display, uint32_t connectorId) :
	mLog("Keyboard")
{
	mSeatKeyboard = display.getSeat()->getKeyboard();

	if (!mSeatKeyboard)
	{
		throw InputItf::Exception("No seat keyboard");
	}

	mSurface = dynamic_pointer_cast<Connector>(
		display.getConnectorById(connectorId))->getSurface();

	LOG(mLog, DEBUG) << "Create";
}

Keyboard::~Keyboard()
{
	mSeatKeyboard->clearCallbacks(mSurface);

	LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Keyboard::setCallbacks(const KeyboardCallbacks& callbacks)
{
	LOG(mLog, DEBUG) << "setCallbacks";

	mSeatKeyboard->setCallbacks(mSurface, callbacks);
}

}
