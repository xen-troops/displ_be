/*
 *  Input Interface
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#ifndef SRC_INPUTITF_HPP_
#define SRC_INPUTITF_HPP_

#include <exception>
#include <memory>

namespace InputItf {

/***************************************************************************//**
 * @defgroup input_itf Input interface
 * Abstract classes for input devices implementation.
 ******************************************************************************/

/***************************************************************************//**
 * Exception generated by input interface.
 * @ingroup input_itf
 ******************************************************************************/
class Exception : public std::exception
{
public:
	/**
	 * @param msg error message
	 */
	explicit Exception(const std::string& msg) : mMsg(msg) {};
	virtual ~Exception() {}

	/**
	 * returns error message
	 */
	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

struct KeyboardCallbacks
{
	std::function<void(uint32_t key, uint32_t state)> key;
};

struct PointerCallbacks
{
	std::function<void(int32_t x, int32_t y)> move;
};

struct TouchCallbacks
{
	std::function<void(int32_t x, int32_t y)> touch;
};

template<typename T>
class InputDevice
{
public:
	virtual void setCallbacks(const T& callbacks) = 0;

	virtual ~InputDevice() {}
};

}

#endif /* SRC_INPUTITF_HPP_ */
