/*
 *  DRM backend
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

#ifndef INCLUDE_DRMBACKEND_HPP_
#define INCLUDE_DRMBACKEND_HPP_

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

extern "C" {
#include <xen/io/drmif_linux.h>
}

#include "CommandHandler.hpp"

/***************************************************************************//**
 * @defgroup drm_be
 * Backend related classes.
 ******************************************************************************/

/***************************************************************************//**
 * Ring buffer used to send events to the frontend.
 * @ingroup drm_be
 ******************************************************************************/
class ConEventRingBuffer : public XenBackend::RingBufferOutBase<
										xendrm_event_page, xendrm_evt>
{
public:
	/**
	 * @param id        connector id
	 * @param domId     frontend domain id
	 * @param port      event channel port number
	 * @param ref       grant table reference
	 * @param offset    start of the ring buffer inside the page
	 * @param size      size of the ring buffer
	 */
	ConEventRingBuffer(int id, int domId, int port,
					   int ref, int offset, size_t size);

private:
	int mId;
	XenBackend::Log mLog;
};

/***************************************************************************//**
 * Ring buffer used for the connector control.
 * @ingroup drm_be
 ******************************************************************************/
class ConCtrlRingBuffer : public XenBackend::RingBufferInBase<
											xen_drmif_back_ring,
											xen_drmif_sring,
											xendrm_req,
											xendrm_resp>
{
public:
	/**
	 * @param drm         drm device
	 * @param eventBuffer event ring buffer
	 * @param id          connector id
	 * @param domId       frontend domain id
	 * @param port        event channel port number
	 * @param ref         grant table reference
	 */
	ConCtrlRingBuffer(Drm::Device& drm,
					  std::shared_ptr<ConEventRingBuffer> eventBuffer,
					  int id, int domId, int port, int ref);

private:
	int mId;
	CommandHandler mCommandHandler;
	XenBackend::Log mLog;

	void processRequest(const xendrm_req& req);
};

/***************************************************************************//**
 * DRM frontend handler.
 * @ingroup drm_be
 ******************************************************************************/
class DrmFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	/**
	 * @param drmDevice name of drm device
	 * @param domId     frontend domain id
	 * @param backend   backend instance
	 * @param id        frontend instance id
	 */
	DrmFrontendHandler(const std::string& drmDevice, int domId,
					   XenBackend::BackendBase& backend, int id) :
		FrontendHandlerBase(domId, backend, id),
		mDrm(drmDevice),
		mLog("DrmFrontend") {}

protected:

	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind();

private:

	Drm::Device mDrm;
	XenBackend::Log mLog;

	void createConnector(const std::string& streamPath, int conId);
};

/***************************************************************************//**
 * DRM backend class.
 * @ingroup drm_be
 ******************************************************************************/
class DrmBackend : public XenBackend::BackendBase
{
public:
	/**
	 * @param domId         domain id
	 * @param deviceName    device name
	 * @param id            instance id
	 */
	DrmBackend(int domId, const std::string& deviceName, int id);

protected:

	/**
	 * Is called when new DRM frontend appears.
	 * @param domId domain id
	 * @param id    instance id
	 */
	void onNewFrontend(int domId, int id);
};

#endif /* INCLUDE_DRMBACKEND_HPP_ */
