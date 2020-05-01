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

#include "Compositor.hpp"
#include "ConnectorBase.hpp"
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
class Connector : public ConnectorBase
{
public:

	virtual ~Connector();

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
	bool isInitialized() const override { return mSurface != nullptr; }

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

protected:

	CompositorPtr mCompositor;

	void onInit(SurfacePtr surface, DisplayItf::FrameBufferPtr frameBuffer);
	void onRelease();

private:

	friend class Display;
	friend class ShellConnector;
	friend class IviConnector;

	Connector(const std::string& name, CompositorPtr compositor);

	std::string mName;

	SurfacePtr mSurface;
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
		if (!mShellSurface)
		{
			mShellSurface = mShell->createShellSurface(mCompositor->createSurface());

			mShellSurface->setTopLevel();
		}

		onInit(mShellSurface->getSurface(), frameBuffer);
	}

	/**
	 * Releases initialized connector
	 */
	void release() override
	{
		mShellSurface.reset();

		onRelease();
	}

private:

	friend class Display;

	ShellConnector(const std::string& name, ShellPtr shell,
				   CompositorPtr compositor) :
		Connector(name, compositor),
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
		if (!mIviSurface)
		{
			mIviSurface = mIviApplication->createIviSurface(
					mCompositor->createSurface(), mSurfaceId);
		}

		onInit(mIviSurface->getSurface(), frameBuffer);
	}

	/**
	 * Releases initialized connector
	 */
	void release() override
	{
		mIviSurface.reset();

		onRelease();
	}

private:

	friend class Display;

	IviConnector(const std::string& name, IviApplicationPtr iviApplication,
				 CompositorPtr compositor, uint32_t surfaceId) :
		Connector(name, compositor),
		mIviApplication(iviApplication),
		mSurfaceId(surfaceId) {}

	IviApplicationPtr mIviApplication;
	IviSurfacePtr mIviSurface;
	uint32_t mSurfaceId;
};
#endif

typedef std::shared_ptr<Connector> ConnectorPtr;

}

#endif /* SRC_WAYLAND_CONNECTOR_HPP_ */
