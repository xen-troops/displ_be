/*
 *  WaylandZCopy class
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
#include "wayland-drm-client-protocol.h"

namespace Wayland {

/***************************************************************************//**
 * Wayland ZCopy class.
 * @ingroup wayland
 ******************************************************************************/
class WaylandZCopy : public Registry
{
public:

	bool isZeroCopySupported();

	DisplayItf::DisplayBufferPtr createDumb(
			uint32_t width, uint32_t height, uint32_t bpp,
			domid_t domId, DisplayItf::GrantRefs& refs, bool allocRefs);

protected:

	bool mIsAuthenticated;
	XenBackend::Log mLog;

	std::unique_ptr<Drm::DisplayZCopy> mDrmDevice;

	std::mutex mMutex;

	WaylandZCopy(wl_registry* registry, uint32_t id, uint32_t version);

	virtual void authenticate() = 0;

	void onDevice(const std::string& name);
	void onFormat(uint32_t format);
	void onAuthenticated();
};

/***************************************************************************//**
 * Wayland DRM class.
 * @ingroup wayland
 ******************************************************************************/
class WaylandDrm : public WaylandZCopy
{
public:

	~WaylandDrm();

	DisplayItf::FrameBufferPtr createDrmBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat);

private:

	wl_drm* mWlDrm;
	wl_drm_listener mWlListener;

	friend class Display;

	WaylandDrm(wl_registry* registry, uint32_t id, uint32_t version);

	static void sOnDevice(void *data, wl_drm *drm, const char *name);
	static void sOnFormat(void *data, wl_drm *drm, uint32_t format);
	static void sOnAuthenticated(void *data, wl_drm *drm);
	static void sOnCapabilities(void *data, wl_drm *drm, uint32_t value);

	void onCapabilities(uint32_t value);

	virtual void authenticate() override;

	void init();
	void release();
};

typedef std::shared_ptr<WaylandDrm> WaylandDrmPtr;

/***************************************************************************//**
 * Wayland KMS class.
 * @ingroup wayland
 ******************************************************************************/
class WaylandKms : public WaylandZCopy
{
public:

	~WaylandKms();

	DisplayItf::FrameBufferPtr createKmsBuffer(
			DisplayItf::DisplayBufferPtr displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat);

private:

	wl_kms* mWlKms;
	wl_kms_listener mWlListener;

	friend class Display;

	WaylandKms(wl_registry* registry, uint32_t id, uint32_t version);

	static void sOnDevice(void *data, wl_kms *kms, const char *name);
	static void sOnFormat(void *data, wl_kms *kms, uint32_t format);
	static void sOnAuthenticated(void *data, wl_kms *kms);

	virtual void authenticate() override;

	void init();
	void release();
};

typedef std::shared_ptr<WaylandKms> WaylandKmsPtr;

}

#endif /* SRC_WAYLAND_WAYLANDDRM_HPP_ */
