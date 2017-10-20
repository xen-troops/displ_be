/*
 * IviSurface.hpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_IVISURFACE_HPP_
#define SRC_WAYLAND_IVISURFACE_HPP_

#include <memory>

#include <ilm/ivi-application-client-protocol.h>

#include <xen/be/Log.hpp>

#include "Surface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Wayland IVI surface class.
 * @ingroup wayland
 ******************************************************************************/
class IviSurface
{
public:

	~IviSurface();

	/**
	 * Returns associated surface
	 */
	SurfacePtr getSurface() const { return mSurface; }

	/**
	 * Returns IVI surface ID
	 */
	uint32_t getIlmId() const { return mIlmSurfaceId; }

private:

	friend class Display;
	friend class IviApplication;

	IviSurface(ivi_application* iviApplication, SurfacePtr surface,
			   uint32_t surfaceId);

	ivi_surface* mWlIviSurface;
	uint32_t mIlmSurfaceId;
	SurfacePtr mSurface;

	XenBackend::Log mLog;

	void init(ivi_application* iviApplication);
	void release();
};

typedef std::shared_ptr<IviSurface> IviSurfacePtr;

}

#endif /* SRC_WAYLAND_IVISURFACE_HPP_ */
