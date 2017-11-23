/*
 *  SurfaceManager class
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

#include "SurfaceManager.hpp"

#include "Exception.hpp"

using std::find;
using std::find_if;
using std::lock_guard;
using std::mutex;
using std::pair;
using std::string;

namespace Wayland {

/*******************************************************************************
 * ConnectorManager
 ******************************************************************************/

SurfaceManager::SurfaceManager() :
	mLog("SurfaceManager")
{

}

SurfaceManager& SurfaceManager::getInstance()
{
	static SurfaceManager sConnectorManager;

	return sConnectorManager;
}

void SurfaceManager::createSurface(const string& connectorName,
								   wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Create surface: " << connectorName;

	mSurfaces[connectorName] = surface;

	for(auto subscriber : mSubscribers)
	{
		subscriber->onSurfaceCreate(connectorName, surface);
	}
}

void SurfaceManager::deleteSurface(const string& connectorName,
								   wl_surface* surface)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Delete surface: " << connectorName;

	for(auto subscriber : mSubscribers)
	{
		subscriber->onSurfaceDelete(connectorName, surface);
	}

	mSurfaces.erase(connectorName);
}

wl_surface* SurfaceManager::getSurfaceByConnectorName(const string& connectorName)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mSurfaces.find(connectorName);

	if (it != mSurfaces.end())
	{
		return it->second;
	}

	return nullptr;
}

string SurfaceManager::getConnectorNameBySurface(wl_surface* surface)
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

void SurfaceManager::subscribe(SurfaceNotificationItf* subscriber)
{
	lock_guard<mutex> lock(mMutex);

	if (find(mSubscribers.begin(), mSubscribers.end(), subscriber) ==
		mSubscribers.end())
	{
		mSubscribers.push_back(subscriber);
	}

}

void SurfaceManager::unsubscribe(SurfaceNotificationItf* subscriber)
{
	lock_guard<mutex> lock(mMutex);

	auto it = find(mSubscribers.begin(), mSubscribers.end(), subscriber);

	if (it != mSubscribers.end())
	{
		mSubscribers.erase(it);
	}
}

}
