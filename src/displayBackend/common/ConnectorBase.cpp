/*
 *  Base connector class
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
 * Copyright (C) 2020 EPAM Systems Inc.
 *
 */

#include <endian.h>
#include <xen/be/Exception.hpp>

#include "ConnectorBase.hpp"
#include "drm_edid.h"
#include "Edid.hpp"

using XenBackend::Exception;
using XenBackend::XenGnttabBuffer;

ConnectorBase::ConnectorBase(domid_t domId, uint32_t width, uint32_t height) :
	mDomId(domId),
	mCfgWidth(width),
	mCfgHeight(height),
	mLog("Connector")
{
}

/*******************************************************************************
 * Protected
 ******************************************************************************/
size_t ConnectorBase::getEDID(grant_ref_t startDirectory, uint32_t size) const
{
	GrantRefs refs;

	pgDirGetBufferRefs(mDomId, startDirectory, size, refs);
	if (!refs.size())
	{
		throw Exception("Cannot get grant references of the EDID buffer",
						EINVAL);
	}

	XenGnttabBuffer edidBuffer(mDomId, refs.data(), refs.size());

	memset(edidBuffer.get(), 0, XENDISPL_EDID_BLOCK_SIZE);

	auto edidBlock = static_cast<edid*>(edidBuffer.get());

	Edid::putEssentials(edidBlock);
	Edid::putColorSpace(edidBlock);
	Edid::putTimings(edidBlock);
	Edid::putDetailedTiming(edidBlock, 0, mCfgWidth, mCfgHeight, Edid::EDID_DPI);
	Edid::putDisplayDescritor(edidBlock, 1);
	Edid::putBlockCheckSum(static_cast<uint8_t*>(edidBuffer.get()));

	return XENDISPL_EDID_BLOCK_SIZE;
}

