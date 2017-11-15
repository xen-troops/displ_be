/*
 *  ConnectorManager class
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
 *
 */

#include "ConnectorManager.hpp"

#include "Exception.hpp"

using std::find_if;
using std::lock_guard;
using std::mutex;
using std::pair;
using std::string;

namespace Wayland {

/*******************************************************************************
 * ConnectorManager
 ******************************************************************************/

ConnectorManager& ConnectorManager::getInstance()
{
	static ConnectorManager sConnectorManager;

	return sConnectorManager;
}

void ConnectorManager::createConnector(const string& name, wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	mSurfaces[name] = surface;

	for(auto subscriber : mSubscribers)
	{
		subscriber->onConnectorCreate(name, surface);
	}
}

void ConnectorManager::deleteConnector(const string& name, wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	for(auto subscriber : mSubscribers)
	{
		subscriber->onConnectorDelete(name, surface);
	}

	mSurfaces.erase(name);
}

wl_surface* ConnectorManager::getSurfaceByName(const string& name)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mSurfaces.find(name);

	if (it != mSurfaces.end())
	{
		return it->second;
	}

	return nullptr;
}

string ConnectorManager::getNameBySurface(wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	string result;

	auto it = find_if(mSurfaces.begin(), mSurfaces.end(),
					  [&surface](const pair<string, wl_surface*>& value)
					  { return value.second == surface; });

	if (it != mSurfaces.end())
	{
		result = it->first;
	}

	return result;
}

}
