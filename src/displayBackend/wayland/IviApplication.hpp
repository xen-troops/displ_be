/*
 * IviApplication.hpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_IVIAPPLICATION_HPP_
#define SRC_WAYLAND_IVIAPPLICATION_HPP_

#include <xen/be/Log.hpp>

#include "IviSurface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland IVI Applicaion class.
 * @ingroup wayland
 ******************************************************************************/
class IviApplication
{
public:

	~IviApplication();

	/**
	 * Creates IVI surface
	 * @param surface surface
	 */
	IviSurfacePtr createIviSurface(SurfacePtr surface, uint32_t width,
								   uint32_t height, uint32_t pixelFormat);

private:

	friend class Display;

	IviApplication(wl_display* display);

	bool mInitialised;
	XenBackend::Log mLog;

	void init(wl_display* display);
	void release();
};

typedef std::shared_ptr<IviApplication> IviApplicationPtr;

}

#endif /* SRC_WAYLAND_IVIAPPLICATION_HPP_ */
