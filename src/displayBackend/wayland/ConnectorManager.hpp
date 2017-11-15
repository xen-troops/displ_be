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

#ifndef SRC_WAYLAND_CONNECTORMANAGER_HPP_
#define SRC_WAYLAND_CONNECTORMANAGER_HPP_

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Surface.hpp"

namespace Wayland {

class ConnectorNotificationItf
{
public:
	virtual ~ConnectorNotificationItf() {};
	virtual void onConnectorCreate(const std::string& name,
								   wl_surface* surface) = 0;
	virtual void onConnectorDelete(const std::string& name,
								   wl_surface* surface) = 0;
};

class ConnectorManager
{
public:

	ConnectorManager(const ConnectorManager&) = delete;
	void operator=(const ConnectorManager&) = delete;

	static ConnectorManager& getInstance();

	void createConnector(const std::string& name, wl_surface* surface);
	void deleteConnector(const std::string& name, wl_surface* surface);

	wl_surface* getSurfaceByName(const std::string& name);
	std::string getNameBySurface(wl_surface* surface);

	void subscribe(ConnectorNotificationItf* subscriber);
	void unsubscribe(ConnectorNotificationItf* subscriber);

private:

	ConnectorManager() = default;

	std::list<ConnectorNotificationItf*> mSubscribers;
	std::unordered_map<std::string, wl_surface*> mSurfaces;
	std::mutex mMutex;
};

}

#endif /* SRC_WAYLAND_CONNECTORMANAGER_HPP_ */
