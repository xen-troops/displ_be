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

namespace Wayland {

template<typename T>
class SeatDevice
{
public:

	SeatDevice()
	{
		mCurrentCallback = mCallbacks.end();
	}

	void setCallbacks(SurfacePtr surface, const T& callbacks)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mCallbacks[surface->mWlSurface] = callbacks;
	}

	void clearCallbacks(SurfacePtr surface)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		wl_surface* currentSurface = nullptr;

		if (mCurrentCallback != mCallbacks.end())
		{
			currentSurface = mCurrentCallback->first;
		}

		mCallbacks.erase(surface->mWlSurface);

		mCurrentCallback = mCallbacks.find(currentSurface);
	}

protected:

	typedef typename std::unordered_map<wl_surface*, T>::iterator CallbackIt;

	std::mutex mMutex;
	std::unordered_map<wl_surface*, T> mCallbacks;
	CallbackIt mCurrentCallback;
};

}

#endif /* SRC_WAYLAND_SEATDEVICE_HPP_ */
