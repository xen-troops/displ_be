/*
 *  Kms class
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

#ifndef SRC_WAYLAND_WAYLANDKMS_HPP_
#define SRC_WAYLAND_WAYLANDKMS_HPP_

#include <mutex>
#include <string>

#include <xen/be/Log.hpp>

#include "drm/Display.hpp"
#include "Registry.hpp"
#include "wayland-kms-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * Wayland KMS class.
 * @ingroup wayland
 ******************************************************************************/
class WaylandKms : public Registry
{
public:

	~WaylandKms();

	bool isZeroCopySupported();

	DisplayItf::DisplayBufferPtr createDumb(
			uint32_t width, uint32_t height, uint32_t bpp,
			domid_t domId, DisplayItf::GrantRefs& refs, bool allocRefs);

	DisplayItf::FrameBufferPtr createKmsBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat);

private:

	friend class Display;

	WaylandKms(wl_registry* registry, uint32_t id, uint32_t version);

	wl_kms* mWlKms;
	bool mIsAuthenticated;
	XenBackend::Log mLog;

	std::unique_ptr<Drm::Display> mDrmDevice;
	wl_kms_listener mWlListener;

	std::mutex mMutex;

	static void sOnDevice(void *data, wl_kms *kms, const char *name);
	static void sOnFormat(void *data, wl_kms *kms, uint32_t format);
	static void sOnAuthenticated(void *data, wl_kms *kms);

	void onDevice(const std::string& name);
	void onFormat(uint32_t format);
	void onAuthenticated();

	void init();
	void release();
};

typedef std::shared_ptr<WaylandKms> WaylandKmsPtr;

}

#endif /* SRC_WAYLAND_WAYLANDKMS_HPP_ */
