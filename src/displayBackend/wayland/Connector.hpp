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
#ifdef WITH_IVI_EXTENSION
#include "IviApplication.hpp"
#include "IviSurface.hpp"
#endif
#include "Shell.hpp"
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
	 * @param width       width
	 * @param height      height
	 * @param frameBuffer frame buffer
	 */
	void init(uint32_t width, uint32_t height,
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
public:
	/**
	 * Initializes connector
	 * @param width       width
	 * @param height      height
	 * @param frameBuffer frame buffer
	 */
	void init(uint32_t width, uint32_t height,
			  DisplayItf::FrameBufferPtr frameBuffer) override
	{
		mShellSurface = mShell->createShellSurface(getSurface());

		Connector::init(width, height, frameBuffer);
	}

	/**
	 * Releases initialized connector
	 */
	void release() override
	{
		Connector::release();

		mShellSurface.reset();
	}

private:

	friend class Display;

	ShellConnector(const std::string& name, ShellPtr shell,
				   SurfacePtr surface) :
		Connector(name, surface),
		mShell(shell) {}

	ShellPtr mShell;
	ShellSurfacePtr mShellSurface;
};

#ifdef WITH_IVI_EXTENSION
/***************************************************************************//**
 * IVI connector
 * @ingroup wayland
 ******************************************************************************/
class IviConnector : public Connector
{
public:
	/**
	 * Initializes connector
	 * @param width       width
	 * @param height      height
	 * @param frameBuffer frame buffer
	 */
	void init(uint32_t width, uint32_t height,
			  DisplayItf::FrameBufferPtr frameBuffer) override
	{
		mIviSurface = mIviApplication->createIviSurface(getSurface());

		Connector::init(width, height, frameBuffer);
	}

	/**
	 * Releases initialized connector
	 */
	void release() override
	{
		Connector::release();

		mIviSurface.reset();
	}

private:

	friend class Display;

	IviConnector(const std::string& name, IviApplicationPtr iviApplication,
				 SurfacePtr surface) :
		Connector(name, surface),
		mIviApplication(iviApplication) {}

	IviApplicationPtr mIviApplication;
	IviSurfacePtr mIviSurface;
};
#endif

typedef std::shared_ptr<Connector> ConnectorPtr;

}

#endif /* SRC_WAYLAND_CONNECTOR_HPP_ */
