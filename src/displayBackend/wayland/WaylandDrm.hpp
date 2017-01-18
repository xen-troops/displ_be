/*
 *  Drm class
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

#ifndef SRC_WAYLAND_WAYLANDDRM_HPP_
#define SRC_WAYLAND_WAYLANDDRM_HPP_

#include <mutex>
#include <string>

#include <xen/be/Log.hpp>

#include "drm/Display.hpp"
#include "Registry.hpp"
#include "wayland-drm/wayland-drm-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * Wayland DRM class.
 * @ingroup wayland
 ******************************************************************************/
class WaylandDrm : public Registry
{
public:

	~WaylandDrm();

	bool isZeroCopySupported();

	DisplayItf::DisplayBufferPtr createDumb(
			domid_t domId, const std::vector<grant_ref_t>& refs,
			uint32_t width, uint32_t height, uint32_t bpp);

	DisplayItf::FrameBufferPtr createDrmBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat);

private:

	friend class Display;

	WaylandDrm(wl_registry* registry, uint32_t id, uint32_t version);

	wl_drm* mWlDrm;
	bool mIsAuthenticated;
	XenBackend::Log mLog;

	std::unique_ptr<Drm::Display> mDrmDevice;
	wl_drm_listener mWlListener;

	std::mutex mMutex;

	static void sOnDevice(void *data, wl_drm *drm, const char *name);
	static void sOnFormat(void *data, wl_drm *drm, uint32_t format);
	static void sOnAuthenticated(void *data, wl_drm *drm);
	static void sOnCapabilities(void *data, wl_drm *drm, uint32_t value);

	void onDevice(const std::string& name);
	void onFormat(uint32_t format);
	void onAuthenticated();
	void onCapabilities(uint32_t value);

	void init();
	void release();
};

typedef std::shared_ptr<WaylandDrm> WaylandDrmPtr;

}

#endif /* SRC_WAYLAND_WAYLANDDRM_HPP_ */
