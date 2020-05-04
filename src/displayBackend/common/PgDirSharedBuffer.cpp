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

#include "PgDirSharedBuffer.hpp"

#include <iomanip>
#include <vector>

#include <xen/be/Exception.hpp>

#include "DisplayItf.hpp"

using std::min;

using XenBackend::XenGnttabBuffer;

void pgDirGetBufferRefs(domid_t domId, grant_ref_t startDirectory,
						uint32_t size, GrantRefs& refs)
{
	XenBackend::Log log("PgDirSharedBuffer");

	refs.clear();

	size_t requestedNumGrefs = (size + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE;

	DLOG(log, DEBUG) << "Get buffer refs, directory: " << startDirectory
					  << ", size: " << size
					  << ", in grefs: " << requestedNumGrefs;

	while(startDirectory != 0 && requestedNumGrefs)
	{
		DLOG(log, DEBUG) << "startDirectory: " << startDirectory;

		XenGnttabBuffer pageBuffer(domId, startDirectory);

		xendispl_page_directory* pageDirectory =
				static_cast<xendispl_page_directory*>(pageBuffer.get());

		size_t numGrefs = min(requestedNumGrefs, (XC_PAGE_SIZE -
							  offsetof(xendispl_page_directory, gref)) /
							  sizeof(uint32_t));

		DLOG(log, DEBUG) << "Gref address: " << pageDirectory->gref
						  << ", numGrefs " << numGrefs;

		refs.insert(refs.end(), pageDirectory->gref,
					pageDirectory->gref + numGrefs);

		requestedNumGrefs -= numGrefs;

		startDirectory = pageDirectory->gref_dir_next_page;
	}

	DLOG(log, DEBUG) << "Get buffer refs, num refs: " << refs.size();
}

void pgDirSetBufferRefs(domid_t domId, grant_ref_t startDirectory,
						uint32_t size, GrantRefs& refs)
{
	XenBackend::Log log("PgDirSharedBuffer");

	size_t requestedNumGrefs = (size + XC_PAGE_SIZE - 1) / XC_PAGE_SIZE;

	DLOG(log, DEBUG) << "Set buffer refs, directory: " << startDirectory
					  << ", size: " << size
					  << ", in grefs: " << requestedNumGrefs;

	grant_ref_t *grefs = refs.data();

	while(startDirectory != 0 && requestedNumGrefs)
	{
		DLOG(log, DEBUG) << "startDirectory: " << startDirectory;

		XenGnttabBuffer pageBuffer(domId, startDirectory);

		xendispl_page_directory* pageDirectory =
				static_cast<xendispl_page_directory*>(pageBuffer.get());

		size_t numGrefs = min(requestedNumGrefs, (XC_PAGE_SIZE -
							  offsetof(xendispl_page_directory, gref)) /
							  sizeof(uint32_t));

		DLOG(log, DEBUG) << "Gref address: " << pageDirectory->gref
						  << ", numGrefs " << numGrefs;

		memcpy(pageDirectory->gref, grefs, numGrefs * sizeof(grant_ref_t));

		requestedNumGrefs -= numGrefs;
		grefs += numGrefs;

		DLOG(log, DEBUG) << "requestedNumGrefs left: " << requestedNumGrefs;

		startDirectory = pageDirectory->gref_dir_next_page;
	}

	DLOG(log, DEBUG) << "Set buffer refs, num refs: " << refs.size();
}
