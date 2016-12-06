/*
 *  Connector class
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

#ifndef SRC_WAYLAND_CONNECTOR_HPP_
#define SRC_WAYLAND_CONNECTOR_HPP_

#include <atomic>
#include <memory>

#include <xen/be/Log.hpp>

#include "ShellSurface.hpp"

#include "DisplayItf.hpp"

namespace Wayland {

class Connector : public ConnectorItf
{
public:

	Connector(std::shared_ptr<ShellSurface> shellSurface,
			  uint32_t id, uint32_t x, uint32_t y,
			  uint32_t width, uint32_t height);

	~Connector();
	/**
	 * Checks if the connector is connected
	 * @return <i>true</i> if connected
	 */
	bool isConnected() const override { return true; }

	/**
	 * Checks if the connector is initialized
	 * @return <i>true</i> if initialized
	 */
	bool isInitialized() const override { return mInitialized; }

	/**
	 * Initializes connector
	 * @param x           horizontal offset
	 * @param y           vertical offset
	 * @param width       width
	 * @param height      height
	 * @param frameBuffer frame buffer
	 */
	void init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
			  std::shared_ptr<FrameBufferItf> frameBuffer) override;

	/**
	 * Releases initialized connector
	 */
	void release() override;

	/**
	 * Performs page flip
	 * @param frameBuffer frame buffer to flip
	 * @param cbk         callback
	 */
	void pageFlip(std::shared_ptr<FrameBufferItf> frameBuffer,
				  FlipCallback cbk) override;

private:
	std::shared_ptr<ShellSurface> mShellSurface;
	uint32_t mX;
	uint32_t mY;
	uint32_t mWidth;
	uint32_t mHeight;
	std::atomic_bool mInitialized;
	XenBackend::Log mLog;
};

}

#endif /* SRC_WAYLAND_CONNECTOR_HPP_ */
