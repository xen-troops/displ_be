/*
 * Config.cpp
 *
 *  Created on: Feb 17, 2017
 *      Author: al1
 */

#include "Config.hpp"

using std::string;

using libconfig::ParseException;

Config::Config(const string& fileName) :
	mLog("Config")
{
	try
	{
		mConfig.readFile(fileName.c_str());
	}
	catch(const ParseException& e)
	{
		LOG(mLog, DEBUG) << e.getError()
						 << ", file: " << e.getFile()
						 << ", line: " << e.getLine();

		throw;
	}
}
