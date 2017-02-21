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

#include "Config.hpp"

using std::string;
using std::to_string;

using libconfig::FileIOException;
using libconfig::ParseException;

/*******************************************************************************
 * Config
 ******************************************************************************/

Config::Config(const string& fileName) :
	mLog("Config")
{
	const char* cfgName = cDefaultCfgName;

	try
	{
		if (!fileName.empty())
		{
			cfgName = fileName.c_str();
		}

		LOG(mLog, DEBUG) << "Open file: " << cfgName;

		mConfig.readFile(cfgName);
	}
	catch(const FileIOException& e)
	{
		throw ConfigException("Can't open config file: " + string(cfgName));
	}
	catch(const ParseException& e)
	{
		throw ConfigException("Config: " + string(e.getError()) +
							  ", file: " + string(e.getFile()) +
							  ", line: " + to_string(e.getLine()));
	}
}

/*******************************************************************************
 * Public
 ******************************************************************************/

Config::DisplayMode Config::displayMode()
{
	return DisplayMode::WAYLAND;
//	return DisplayMode::DRM;
}

bool Config::wlBackground(uint32_t& w, uint32_t& h)
{
	w = 800;
	h = 600;

	return true;
}

int Config::wlConnectorsCount()
{
	return 2;
}

void Config::wlConnector(int idx, string& name, uint32_t& displ,
						 uint32_t& x, uint32_t& y,
						 uint32_t& w, uint32_t& h, uint32_t& z)
{
	if (idx == 0)
	{
		name = "First";
		displ = 0;
		x = 0;
		y = 0;
		w = 400;
		h = 600;
		z = 0;
	}
	else
	{
		name = "Second";
		displ = 0;
		x = 400;
		y = 0;
		w = 400;
		h = 600;
		z = 1;
	}
}

int Config::inputKeyboardsCount()
{
	return 2;
}

void Config::inputKeyboard(int idx, int& id, bool& wayland, string& name)
{
	if (idx == 0)
	{
		id = 0;
		wayland = false;
		name = "/dev/input/event0";
	}
	else
	{
		id = 0;
		wayland = true;
		name = "First";
	}
}

int Config::inputPointersCount()
{
	return 2;
}

void Config::inputPointer(int idx, int& id, bool& wayland, string& name)
{
	if (idx == 0)
	{
		id = 0;
		wayland = false;
		name = "/dev/input/event1";
	}
	else
	{
		id = 0;
		wayland = true;
		name = "First";
	}
}

int Config::inputTouchesCount()
{
	return 2;
}

void Config::inputTouch(int idx, int& id, bool& wayland, string& name)
{
	if (idx == 0)
	{
		id = 0;
		wayland = false;
		name = "/dev/input/event2";
	}
	else
	{
		id = 0;
		wayland = true;
		name = "First";
	}
}

std::string Config::domConnectorName(const std::string& domName, uint16_t devId,
							 int idx)
{
	if (idx == 0)
	{
		return "First";
	}
	else if (idx == 1)
	{
		return "Second";
	}

	throw ConfigException("Connector not found.");
}

bool Config::domKeyboardId(const std::string& domName, uint16_t devId, int& id)
{
	id = devId;

	return true;
}

bool Config::domPointerId(const std::string& domName, uint16_t devId, int& id)
{
	id = devId;

	return true;
}

bool Config::domTouchId(const std::string& domName, uint16_t devId, int& id)
{
	id = devId;

	return true;
}
