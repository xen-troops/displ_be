/*
 * IviApplication.hpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_IVIAPPLICATION_HPP_
#define SRC_WAYLAND_IVIAPPLICATION_HPP_

#include <xen/be/Log.hpp>

#include "Registry.hpp"
#include "IviSurface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland IVI Applicaion class.
 * @ingroup wayland
 ******************************************************************************/
class IviApplication : public Registry
{
public:

	~IviApplication();

	/**
	 * Creates IVI surface
	 * @param surface surface
	 */
	IviSurfacePtr createIviSurface(SurfacePtr surface);

private:

	friend class Display;

	IviApplication(wl_registry* registry, uint32_t id, uint32_t version);

	ivi_application* mWlIviApplication;
	XenBackend::Log mLog;

	void init();
	void release();
};

typedef std::shared_ptr<IviApplication> IviApplicationPtr;

}

#endif /* SRC_WAYLAND_IVIAPPLICATION_HPP_ */
