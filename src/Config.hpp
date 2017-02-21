/*
 *  Config
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

#ifndef SRC_CONFIG_HPP_
#define SRC_CONFIG_HPP_

#include <exception>
#include <memory>
#include <string>

#include <libconfig.h++>

#include <xen/be/Log.hpp>

class ConfigException : public std::exception
{
public:

	explicit ConfigException(const std::string& msg) : mMsg(msg) {};
	virtual ~ConfigException() {}

	/**
	 * returns error message
	 */
	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};


class Config
{
public:

	enum class DisplayMode
	{
		DRM,
		WAYLAND
	};

	Config(const std::string& fileName);

	DisplayMode displayMode();

	bool wlBackground(uint32_t& w, uint32_t& h);
	int wlConnectorsCount();
	void wlConnector(int idx, std::string& name, uint32_t& displ,
					 uint32_t& x, uint32_t& y,
					 uint32_t& w, uint32_t& h, uint32_t& z);

	int inputKeyboardsCount();
	void inputKeyboard(int idx, int& id, bool& wayland, std::string& name);

	int inputPointersCount();
	void inputPointer(int idx, int& id, bool& wayland, std::string& name);

	int inputTouchesCount();
	void inputTouch(int idx, int& id, bool& wayland, std::string& name);

	std::string domConnectorName(const std::string& domName, uint16_t devId,
								 int idx);

	bool domKeyboardId(const std::string& domName, uint16_t devId, int& id);
	bool domPointerId(const std::string& domName, uint16_t devId, int& id);
	bool domTouchId(const std::string& domName, uint16_t devId, int& id);

private:

	const char* cDefaultCfgName = "displ_be.cfg";

	XenBackend::Log mLog;
	libconfig::Config mConfig;
};

typedef std::shared_ptr<Config> ConfigPtr;

#endif /* SRC_CONFIG_HPP_ */
