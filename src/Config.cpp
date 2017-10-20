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

void Config::displayDomParams(int idx, string& name, uint16_t& devId,
							  int& connectorsCount)
{
	string sectionName = "display.doms";

	try
	{
		Setting& domSetting = mConfig.lookup(sectionName)[idx];

		name = static_cast<const char*>(domSetting.lookup("name"));
		devId = static_cast<int>(domSetting.lookup("devId"));
		connectorsCount = domSetting.lookup("connectors").getLength();

		LOG(mLog, DEBUG) << sectionName << "[" << idx
						 << "] name: " << name
						 << ", devId: " << devId
						 << ", connectors count: " << connectorsCount;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

string Config::displayDomConnectorName(const string& domName,
									   uint16_t devId, int idx)
{
	string sectionName = "display.doms";

	try
	{
		int index;
		Setting& domain = findSettingByDomain(sectionName, domName, devId,
											  index);
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

int Config::inputDomIndex(const string& domName, uint16_t devId) const
{
	string sectionName = "display.doms";

	try
	{
		int index;

		findSettingByDomain(sectionName, domName, devId, index);

		LOG(mLog, DEBUG) << "Dom name: " << domName << ", dev id: " << devId
						 << ", index: " << index;

		return index;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

void Config::inputKeyboard(int idx, string& device, string& connector)
{
	readInputParams(idx, "keyboard", device, connector);
}

void Config::inputPointer(int idx, string& device, string& connector)
{
	readInputParams(idx, "pointer", device, connector);
}

void Config::inputTouch(int idx, string& device, string& connector)
{
	readInputParams(idx, "touch", device, connector);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Config::initCachedValues()
{
	mDisplayMode = readDisplayMode();
	mDisplayDomainsCount = readSectionCount("display.doms");
	mInputDomainsCount = readSectionCount("input.doms");
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

int Config::readSectionCount(const string& sectionName)
{
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

void Config::readInputParams(int idx, const string& paramName,
		 	 	 	 	 	 string& device, string& connector)
{
	string sectionName = "input.doms";

	try
	{
		device.clear();
		connector.clear();

		Setting& domSetting = mConfig.lookup(sectionName)[idx];

		if (domSetting.exists(paramName))
		{
			Setting& inputSetting = domSetting.lookup(paramName);

			inputSetting.lookupValue("device", device);
			inputSetting.lookupValue("connector", connector);
		}

		LOG(mLog, DEBUG) << sectionName << "[" << idx << "]." << paramName
						 << " device: " << device
						 << ", connector: " << connector;
	}
	catch(const SettingException& e)
	{
		throw ConfigException("Config: error reading " + sectionName);
	}
}

Setting& Config::findSettingByDomain(const string& sectionName,
									 const string& domName, uint16_t devId,
									 int& index) const
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
				index = i;

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
