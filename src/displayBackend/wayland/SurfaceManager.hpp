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

#ifndef SRC_WAYLAND_SURFACEMANAGER_HPP_
#define SRC_WAYLAND_SURFACEMANAGER_HPP_

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Surface.hpp"

namespace Wayland {

class SurfaceNotificationItf
{
public:
	virtual ~SurfaceNotificationItf() {};
	virtual void onSurfaceCreate(const std::string& connectorName,
								 wl_surface* surface) = 0;
	virtual void onSurfaceDelete(const std::string& connectorName,
								 wl_surface* surface) = 0;
};

class SurfaceManager
{
public:

	SurfaceManager(const SurfaceManager&) = delete;
	void operator=(const SurfaceManager&) = delete;

	static SurfaceManager& getInstance();

	void createSurface(const std::string& connectorName, wl_surface* surface);
	void deleteSurface(const std::string& connectorName, wl_surface* surface);

	wl_surface* getSurfaceByConnectorName(const std::string& connectorName);
	std::string getConnectorNameBySurface(wl_surface* surface);

	void subscribe(SurfaceNotificationItf* subscriber);
	void unsubscribe(SurfaceNotificationItf* subscriber);

private:

	SurfaceManager();

	std::list<SurfaceNotificationItf*> mSubscribers;
	std::unordered_map<std::string, wl_surface*> mSurfaces;

	XenBackend::Log mLog;

	std::mutex mMutex;
};

}

#endif /* SRC_WAYLAND_SURFACEMANAGER_HPP_ */
