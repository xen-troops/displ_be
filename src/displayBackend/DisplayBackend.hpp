/*
 *  Display backend
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
 */

#ifndef DISPLAYBACKEND_HPP_
#define DISPLAYBACKEND_HPP_

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

#include "DisplayCommandHandler.hpp"

/***************************************************************************//**
 * @defgroup displ_be Display backend
 * Backend related classes.
 ******************************************************************************/

enum class DisplayMode
{
	DRM,
	WAYLAND
};

/***************************************************************************//**
 * Ring buffer used for the connector control.
 * @ingroup displ_be
 ******************************************************************************/
class CtrlRingBuffer : public XenBackend::RingBufferInBase<
		xen_displif_back_ring, xen_displif_sring, xendispl_req, xendispl_resp>
{
public:
	/**
	 * @param connector      connector object
	 * @param buffersStorage buffers storage
	 * @param eventBuffer    event ring buffer
	 * @param domId          frontend domain id
	 * @param port           event channel port number
	 * @param ref            grant table reference
	 */
	CtrlRingBuffer(ConnectorPtr connector,
				   BuffersStoragePtr buffersStorage,
				   EventRingBufferPtr eventBuffer,
				   domid_t domId, evtchn_port_t port, grant_ref_t ref);

private:

	DisplayCommandHandler mCommandHandler;
	XenBackend::Log mLog;

	void processRequest(const xendispl_req& req);
};

typedef std::shared_ptr<CtrlRingBuffer> CtrlRingBufferPtr;

/***************************************************************************//**
 * Display frontend handler.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param display   display
	 * @param domId     frontend domain id
	 * @param backend   backend instance
	 * @param id        frontend instance id
	 */
	DisplayFrontendHandler(DisplayPtr display,
						   XenBackend::BackendBase& backend,
						    domid_t domId, int id) :
		FrontendHandlerBase("DisplFrontend", backend, domId, id),
		mCurrentConId(0),
		mDisplay(display),
		mBuffersStorage(new BuffersStorage(domId, display)),
		mLog("DisplayFrontend") {}

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind();

private:

	uint32_t mCurrentConId;
	DisplayPtr mDisplay;
	BuffersStoragePtr mBuffersStorage;
	XenBackend::Log mLog;

	void createConnector(const std::string& streamPath, int conId);
};

/***************************************************************************//**
 * Display backend class.
 * @ingroup displ_be
 ******************************************************************************/
class DisplayBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param domId         domain id
	 * @param deviceName    device name
	 * @param id            instance id
	 */
	DisplayBackend(DisplayPtr display, const std::string& deviceName,
				   domid_t domId, int id);

protected:

	/**
	 * Is called when new display frontend appears.
	 * @param domId domain id
	 * @param id    instance id
	 */
	void onNewFrontend(domid_t domId, int id);

private:

	DisplayPtr mDisplay;
};

#endif /* DISPLAYBACKEND_HPP_ */
