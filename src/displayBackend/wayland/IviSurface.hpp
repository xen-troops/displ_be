/*
 * IviSurface.hpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_IVISURFACE_HPP_
#define SRC_WAYLAND_IVISURFACE_HPP_

#include <memory>
#include <unordered_map>

#include <wayland-client.h>
#include <ilm/ilm_types.h>

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

private:

	friend class Display;
	friend class IviApplication;

	IviSurface(SurfacePtr surface, uint32_t width, uint32_t height,
			   uint32_t pixelFormat);

	t_ilm_surface mIlmSurface;
	SurfacePtr mSurface;

	XenBackend::Log mLog;

	static std::unordered_map<uint32_t, ilmPixelFormat> sPixelFormatMap;

	void init(uint32_t width, uint32_t height, uint32_t pixelFormat);
	void release();

	ilmPixelFormat convertPixelFormat(uint32_t pixelFormat);
};

typedef std::shared_ptr<IviSurface> IviSurfacePtr;

}

#endif /* SRC_WAYLAND_IVISURFACE_HPP_ */
