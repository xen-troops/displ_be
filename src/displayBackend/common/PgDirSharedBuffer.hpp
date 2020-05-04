/*
 *  Page directory based shared buffer handling
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
 */

#ifndef SRC_PGDIR_SHARED_BUFFER_HPP_
#define SRC_PGDIR_SHARED_BUFFER_HPP_

extern "C" {
#include <xenctrl.h>
#include <xengnttab.h>
}

#include <xen/be/XenGnttab.hpp>

typedef std::vector<uint32_t> GrantRefs;

void pgDirGetBufferRefs(domid_t domId, grant_ref_t startDirectory,
						uint32_t size, GrantRefs& refs);

void pgDirSetBufferRefs(domid_t domId, grant_ref_t startDirectory,
						uint32_t size, GrantRefs& refs);

#endif /* SRC_PGDIR_SHARED_BUFFER_HPP_ */
