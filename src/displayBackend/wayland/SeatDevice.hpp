/*
 * SeatDevice.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SEATDEVICE_HPP_
#define SRC_WAYLAND_SEATDEVICE_HPP_

#include <mutex>
#include <unordered_map>

#include "Surface.hpp"
#include "SurfaceManager.hpp"

namespace Wayland {

template<typename T>
class SeatDevice : public SurfaceNotificationItf
{
public:

	SeatDevice()
	{
		SurfaceManager::getInstance().subscribe(this);

		mCurrentCallback = mSurfaceCallbacks.end();
	}

	~SeatDevice()
	{
		SurfaceManager::getInstance().unsubscribe(this);
	}

	void setConnectorCallbacks(const std::string& connector, const T& callbacks)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mConnectorCallbacks[connector] = callbacks;

		auto surface = SurfaceManager::getInstance().getSurfaceByConnectorName(connector);

		if (surface)
		{
			setSurfaceCallbacks(surface, callbacks);
		}
	}

	void clearConnectorCallbacks(const std::string& connector)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		auto surface = SurfaceManager::getInstance().getSurfaceByConnectorName(connector);

		if (surface)
		{
			clearSurfaceCallbacks(surface);
		}

		mConnectorCallbacks.erase(connector);
	}

protected:

	typedef typename std::unordered_map<wl_surface*, T>::iterator CallbackIt;

	std::mutex mMutex;
	std::unordered_map<wl_surface*, T> mSurfaceCallbacks;
	std::unordered_map<std::string, T> mConnectorCallbacks;
	CallbackIt mCurrentCallback;

private:

	void onSurfaceCreate(const std::string& connectorName,
						 wl_surface* surface) override
	{
		std::lock_guard<std::mutex> lock(mMutex);

		auto it = mConnectorCallbacks.find(connectorName);

		if (it != mConnectorCallbacks.end())
		{
			setSurfaceCallbacks(surface, it->second);
		}
	}

	void onSurfaceDelete(const std::string& name, wl_surface* surface) override
	{
		std::lock_guard<std::mutex> lock(mMutex);

		auto it = mConnectorCallbacks.find(name);

		if (it != mConnectorCallbacks.end())
		{
			clearSurfaceCallbacks(surface);
		}
	}

	void setSurfaceCallbacks(wl_surface* surface, const T& callbacks)
	{
		mSurfaceCallbacks[surface] = callbacks;
	}

	void clearSurfaceCallbacks(wl_surface* surface)
	{
		if (mCurrentCallback != mSurfaceCallbacks.end() &&
			mCurrentCallback->first == surface)
		{
			mCurrentCallback = mSurfaceCallbacks.end();
		}

		mSurfaceCallbacks.erase(surface);
	}
};

}

#endif /* SRC_WAYLAND_SEATDEVICE_HPP_ */
