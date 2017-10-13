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

using libconfig::Setting;
using libconfig::FileIOException;
using libconfig::ParseException;
using libconfig::SettingException;
using libconfig::SettingNotFoundException;

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

		initCachedValues();
	}
	catch(const FileIOException& e)
	{
		throw ConfigException("Config: can't open file: " + string(cfgName));
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

bool Config::wlBackground(uint32_t& w, uint32_t& h)
{

	string sectionName = "display.wayland.background";

	if (mConfig.exists(sectionName))
	{
		try
		{
			w = mConfig.lookup(sectionName + ".w");
			h = mConfig.lookup(sectionName + ".h");
		}
		catch(const SettingException& e)
		{
			throw ConfigException("Config: error reading " + sectionName);
		}

		LOG(mLog, DEBUG) << "Background, w: " << w << ", h: " << h;

		return true;
	}

	return false;
}

void Config::wlConnector(int idx, string& name, uint32_t& screen,
						 uint32_t& x, uint32_t& y,
						 uint32_t& w, uint32_t& h, uint32_t& z)
{
	string sectionName = "display.wayland.connectors";

	try
	{
		Setting& conSetting = mConfig.lookup(sectionName)[idx];

		name = static_cast<const char*>(conSetting.lookup("name"));

		if (!conSetting.lookupValue("screen", screen))
		{
			screen = 0;
		}

		x = conSetting.lookup("x");
		y = conSetting.lookup("y");
		w = conSetting.lookup("w");
		h = conSetting.lookup("h");

		if (!conSetting.lookupValue("z", z))
		{
			z = 0;
		}

		LOG(mLog, DEBUG) << sectionName << "[" << idx << "] name: " << name
						 << ", screen: " << screen
						 << ", x: " << x << ", y: " << y
						 << ", w: " << w << ", h: " << h << ", z: " << z;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

void Config::inputKeyboard(int idx, int& id, bool& wayland, string& name)
{
	readInputSection("input.keyboards", idx, id, wayland, name);
}

void Config::inputPointer(int idx, int& id, bool& wayland, string& name)
{
	readInputSection("input.pointers", idx, id, wayland, name);
}

void Config::inputTouch(int idx, int& id, bool& wayland, string& name)
{
	readInputSection("input.touches", idx, id, wayland, name);
}

std::string Config::domConnectorName(const std::string& domName, uint16_t devId,
							 int idx)
{
	string sectionName = "display.doms";

	try
	{
		Setting& domain = findSettingByDomain(sectionName, domName, devId);
		Setting& connectors = domain.lookup("connectors");

		if ((idx >= connectors.getLength()) || (idx < 0))
		{
			throw ConfigException("Config: connector index out of range");
		}

		string conName = static_cast<const char*>(connectors[idx]);

		LOG(mLog, DEBUG) << "Dom name: " << domName << ", dev id: " << devId
						 << ", con idx: " << idx << ", con name: " << conName;

		return conName;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

bool Config::domKeyboardId(const std::string& domName, uint16_t devId, int& id)
{
	return readInputId("keyboardId", domName, devId, id);
}

bool Config::domPointerId(const std::string& domName, uint16_t devId, int& id)
{
	return readInputId("pointerId", domName, devId, id);
}

bool Config::domTouchId(const std::string& domName, uint16_t devId, int& id)
{
	return readInputId("touchId", domName, devId, id);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Config::initCachedValues()
{
	mDisplayMode = readDisplayMode();
	mWlConnectorsCount = readWlConnectorsCount();
	mInputKeyboardsCount = readInputsCount("input.keyboards");
	mInputPointersCount = readInputsCount("input.pointers");
	mInputTouchesCount = readInputsCount("input.touches");
}

Config::DisplayMode Config::readDisplayMode()
{
	bool useWayland;

	if (mConfig.lookupValue("display.useWayland", useWayland))
	{
		if (useWayland)
		{
			LOG(mLog, DEBUG) << "useWayland setting set to true: "
							 << "Wayland mode will be used";

			return DisplayMode::WAYLAND;
		}
	}

	LOG(mLog, DEBUG) << "useWayland setting not found or set to false: "
					 << "DRM mode will be used";

	return DisplayMode::DRM;
}

int Config::readWlConnectorsCount()
{
	if (mDisplayMode == DisplayMode::WAYLAND)
	{
		string sectionName = "display.wayland.connectors";

		try
		{
			auto count = mConfig.lookup(sectionName).getLength();

			LOG(mLog, DEBUG) << sectionName << " count: " << count;

			return count;
		}
		catch(const SettingNotFoundException& e)
		{
			throw ConfigException("Config: error reading " + sectionName);
		}
	}
	else
	{
		return 0;
	}
}

void Config::readInputSection(const string& sectionName, int idx, int& id,
							  bool& wayland, string& name)
{
	try
	{
		Setting& setting = mConfig.lookup(sectionName)[idx];

		id = setting.lookup("id");

		wayland = false;

		setting.lookupValue("wayland", wayland);

		if (wayland)
		{
			name = static_cast<const char*>(setting.lookup("conName"));
		}
		else
		{
			name = static_cast<const char*>(setting.lookup("device"));
		}

		LOG(mLog, DEBUG) << sectionName << "[" << idx << "] id: " << id
						 << ", wayland: " << wayland << ", name: " << name;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

int Config::readInputsCount(const std::string& sectionName)
{
	int count = 0;

	if (mConfig.exists(sectionName))
	{
		count = mConfig.lookup(sectionName).getLength();
	}

	LOG(mLog, DEBUG) << sectionName << " count: " << count;

	return count;
}

bool Config::readInputId(const string& settingName, const string& domName,
						 uint16_t devId, int& id)
{
	string sectionName = "input.doms";

	if (mConfig.exists(sectionName))
	{
		Setting& domain = findSettingByDomain(sectionName, domName, devId);

		if (domain.lookupValue(settingName, id))
		{
			LOG(mLog, DEBUG) << "Dom name: " << domName << ", dev id: " << devId
							 << ", " << settingName << ": " << id;

			return true;
		}
	}

	return false;
}

Setting& Config::findSettingByDomain(const string& sectionName,
									 const string& domName, uint16_t devId)
{
	try
	{
		Setting& section = mConfig.lookup(sectionName);

		LOG(mLog, DEBUG) << "Lookup dom name: " << domName
						 << ", dev id: " << devId << " in " << sectionName;

		for(int i = 0; i < section.getLength(); i++)
		{
			string name = static_cast<const char*>(section[i].lookup("name"));
			uint16_t id = static_cast<int>(section[i].lookup("devId"));

			if ((name == domName) && (id == devId))
			{
				return section[i];
			}
		}

		throw ConfigException("Config: no entry found in " + sectionName);
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}
