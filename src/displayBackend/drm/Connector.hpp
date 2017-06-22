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

#ifndef SRC_DRM_CONNECTOR_HPP_
#define SRC_DRM_CONNECTOR_HPP_

#include <atomic>
#include <unordered_map>

#include <xen/be/Log.hpp>

#include "DisplayItf.hpp"
#include "Exception.hpp"
#include "Modes.hpp"

namespace Drm {

extern const uint32_t cInvalidId;

class Display;

/***************************************************************************//**
 * Provides DRM connector functionality.
 * @ingroup drm
 ******************************************************************************/
class Connector : public DisplayItf::Connector
{
public:

	/**
	 * @param device DRM device
	 * @param conId  connector id
	 */
	Connector(Display& device, int conId);

	~Connector();

	/**
	 * Returns assigned CRTC id
	 */
	uint32_t getCrtcId() const { return mCrtcId; }

	/**
	 * Returns connector name
	 */
	std::string getName() const override { return mName; }

	/**
	 * Checks if the connector is connected
	 * @return <i>true</i> if connected
	 */
	bool isConnected() const override { return mConnector->connection ==
										DRM_MODE_CONNECTED; }

	/**
	 * Checks if the connector is initialized and CRTC is assigned
	 * @return <i>true</i> if initialized
	 */
	bool isInitialized() const override { return mCrtcId != cInvalidId; }

	/**
	 * Initializes CRTC mode
	 * @param x           horizontal offset
	 * @param y           vertical offset
	 * @param width       width
	 * @param height      height
	 * @param frameBuffer frame buffer
	 */
	void init(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
			  DisplayItf::FrameBufferPtr frameBuffer) override;

	/**
	 * Releases the previously initialized CRTC mode
	 */
	void release() override;

	/**
	 * Performs page flip
	 * @param frameBuffer frame buffer
	 * @param cbk         callback which will be called when page flip is done
	 */
	virtual void pageFlip(DisplayItf::FrameBufferPtr frameBuffer,
						  FlipCallback cbk) override;

private:

	static std::unordered_map<int, std::string> sConnectorNames;

	Display& mDev;
	int mFd;
	uint32_t mCrtcId;
	ModeConnector mConnector;
	drmModeCrtc* mSavedCrtc;
	std::atomic_bool mFlipPending;
	FlipCallback mFlipCallback;
	XenBackend::Log mLog;
	std::string mName;

	uint32_t findCrtcId();
	uint32_t getAssignedCrtcId();
	uint32_t findMatchingCrtcId();
	bool isCrtcIdUsedByOther(uint32_t crtcId);
	drmModeModeInfoPtr findMode(uint32_t width, uint32_t height);


	friend class Display;

	void flipFinished();
};

typedef std::shared_ptr<Connector> ConnectorPtr;

}

#endif /* SRC_DRM_CONNECTOR_HPP_ */
