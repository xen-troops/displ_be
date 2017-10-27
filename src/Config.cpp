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
using std::vector;

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

		mDisplayMode = readDisplayMode();
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

void Config::getConnectors(vector<Connector>& connectors)
{
	auto sectionName = "display.connectors";

	try
	{
		connectors.clear();

		auto& setting = mConfig.lookup(sectionName);

		for (int i = 0; i < setting.getLength(); i++)
		{
			Connector connector {};

			connector.id =  static_cast<const char*>(setting[i].lookup("id"));
			connector.name = static_cast<const char*>(setting[i].lookup("name"));
			setting[i].lookupValue("surfaceId", connector.surfaceId);

			connectors.push_back(connector);

			LOG(mLog, DEBUG) << sectionName
							 << " Id: " << connector.id
							 << ", name: " << connector.name
							 << ", surfaceId: " << connector.surfaceId;
		}
	}
	catch(const SettingNotFoundException& e)
	{
		throw ConfigException(string("Config: error reading ") + sectionName);
	}
}

void Config::getKeyboards(vector<Input>& keyboards)
{
	getInputs(keyboards, "input.keyboards");
}

void Config::getPointers(vector<Input>& pointers)
{
	getInputs(pointers, "input.pointers");
}

void Config::getTouches(vector<Input>& touches)
{
	getInputs(touches, "input.touches");
}

/*******************************************************************************
 * Private
 ******************************************************************************/

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

void Config::getInputs(vector<Input>& inputs, const string& sectionName)
{
	try
	{
		inputs.clear();

		auto& setting = mConfig.lookup(sectionName);

		for (int i = 0; i < setting.getLength(); i++)
		{
			Input input {};

			input.id =  static_cast<const char*>(setting[i].lookup("id"));
			setting[i].lookupValue("device", input.device);
			setting[i].lookupValue("connector", input.connector);

			LOG(mLog, DEBUG) << sectionName
							 << " id: " << input.id
							 << ", device: " << input.device
							 << ", connector: " << input.connector;
		}
	}
	catch(const SettingNotFoundException& e)
	{
		throw ConfigException(string("Config: error reading ") + sectionName);
	}
}
