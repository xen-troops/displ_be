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

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"
#include "IlmControl.hpp"
#include "IviSurface.hpp"
#include "ShellSurface.hpp"

namespace Wayland {

/***************************************************************************//**
 * Virtual connector class.
 * @ingroup wayland
 ******************************************************************************/
class Connector : public DisplayItf::Connector
{
public:

	virtual ~Connector();

	/**
	 * Returns Surface associated with this connector
	 */
	SurfacePtr getSurface() const { return mSurface; }

	/**
	 * Returns connector name
	 */
	std::string getName() const override { return mName; }

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
			  DisplayItf::FrameBufferPtr frameBuffer) override;

	/**
	 * Releases initialized connector
	 */
	void release() override;

	/**
	 * Performs page flip
	 * @param frameBuffer frame buffer to flip
	 * @param cbk         callback
	 */
	void pageFlip(DisplayItf::FrameBufferPtr frameBuffer,
				  FlipCallback cbk) override;

private:

	friend class Display;
	friend class ShellConnector;
	friend class IviConnector;

	Connector(const std::string& name, SurfacePtr surface);

	std::string mName;
	SurfacePtr mSurface;
	std::atomic_bool mInitialized;
	XenBackend::Log mLog;
};

/***************************************************************************//**
 * Shell connector
 * @ingroup wayland
 ******************************************************************************/
class ShellConnector : public Connector
{
private:

	friend class Display;

	ShellConnector(const std::string& name, ShellSurfacePtr shellSurface) :
		Connector(name, shellSurface->getSurface()),
		mShellSurface(shellSurface) {}

	ShellSurfacePtr mShellSurface;
};

/***************************************************************************//**
 * IVI connector
 * @ingroup wayland
 ******************************************************************************/
class IviConnector : public Connector
{
private:

	friend class Display;

	IviConnector(const std::string& name, IviSurfacePtr iviSurface,
				 IlmControlPtr ilmControl, uint32_t screen,
				 uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t z) :
		Connector(name, iviSurface->getSurface()),
		mIviSurface(iviSurface),
		mIlmControl(ilmControl)
	{
		if (mIlmControl)
		{
			mIlmControl->addSurface(
					mIviSurface->getIlmId(), screen, x, y, w, h, z);
		}
	}

	IviSurfacePtr mIviSurface;
	IlmControlPtr mIlmControl;

	void init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
			  DisplayItf::FrameBufferPtr frameBuffer) override
	{
		Connector::init(x, y, width, height, frameBuffer);

		if (mIlmControl)
		{
			mIlmControl->showSurface(mIviSurface->getIlmId());
		}
	}

	void release() override
	{
		if (mIlmControl)
		{
			mIlmControl->hideSurface(mIviSurface->getIlmId());
		}

		Connector::release();
	}
};

typedef std::shared_ptr<Connector> ConnectorPtr;

}

#endif /* SRC_WAYLAND_CONNECTOR_HPP_ */
