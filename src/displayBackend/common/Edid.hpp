/*
 * Namespace EDIT contains the set of the constants and functions to handle the
 * EDID data.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2020 EPAM Systems Inc.
 *
 */

#ifndef SRC_EDID_HPP_
#define SRC_EDID_HPP_
#include <cstdint>
struct edid;

namespace Edid {
    /* Refresh rate advertized via EDID detailed timings. */
    const int EDID_REFRESH_RATE_HZ = 60;

    /* Dots per inch advertized via EDID detailed timings. */
    const int EDID_DPI = 100;

    /* EDID advertized manufacturing year. */
    const int EDID_MANUFACTURING_YEAR = 2020;

    /**
	 * Calculate and append checksum of the EDID block
	 * @param edidBlock buffer with EDID block
	 */
	void putBlockCheckSum(uint8_t* edidBlock);

	/**
	 * Put essential data into the EDID
	 * @param edidBlock buffer with EDID block
	 */
	void putEssentials(edid* edidBlock);

	/**
	 * Put color space data into the EDID
	 * @param edidBlock buffer with EDID block
	 */
	void putColorSpace(edid* edidBlock);

	/**
	 * * Put established and standard timings into the EDID
	 * * @param edidBlock buffer with EDID block
	 * */
	void putTimings(edid* edidBlock);

	/**
	 * * Put display related data into the EDID
	 * * @param edidBlock buffer with EDID block
	 * * @param descriptorIndex index amid 4 possible
	 * *        18 byte descriptors
	 * */
	void putDisplayDescritor(edid* edidBlock, int descriptorIndex);

	/**
	 * Put detailed timings into the EDID
	 * @param edidBlock buffer with EDID block
	 * @param index     index amid 4 possible 18 byte descriptors
	 * @param xres      desired X resolution
	 * @param yres      desired Y resolution
	 * @param dpi       desired DPI
	 */
	void putDetailedTiming(edid* edidBlock, int index,
       			    uint32_t xres, uint32_t yres,
					uint32_t dpi);

};

#endif

