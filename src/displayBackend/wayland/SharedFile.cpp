/*
 * SharedFile.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: al1
 */

#include "SharedFile.hpp"

#include <cstdlib>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "Exception.hpp"

using std::string;

using XenBackend::XenGnttabBuffer;

namespace Wayland {

/*******************************************************************************
 * SharedFile
 ******************************************************************************/

SharedFile::SharedFile(uint32_t width, uint32_t height, uint32_t bpp,
					   size_t offset, domid_t domId, const GrantRefs& refs) :
	mFd(-1),
	mBuffer(nullptr),
	mWidth(width),
	mHeight(height),
	mStride(4 * ((width * bpp + 31) / 32)),
	mSize(height * mStride),
	mLog("SharedFile")
{
	try
	{
		init(domId, offset, refs);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

SharedFile::~SharedFile()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void SharedFile::copy()
{
	if(!mGnttabBuffer)
	{
		throw Exception("There is no buffer to copy from", ENOENT);
	}

	DLOG("Dumb", DEBUG) << "Copy dumb, handle: " << mFd;

	memcpy(mBuffer, mGnttabBuffer->get(), mSize);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void SharedFile::init(domid_t domId, size_t offset, const GrantRefs& refs)
{
	createTmpFile();

	LOG(mLog, DEBUG) << "Create, w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mStride << ", fd: " << mFd
					 << ", offset: " << offset;

	auto map = mmap(NULL, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);

	if (map == MAP_FAILED)
	{
		throw Exception("Can't map shared file", errno);
	}

	mBuffer = map;

	if (refs.size())
	{
		mGnttabBuffer.reset(
				new XenGnttabBuffer(domId, refs.data(), refs.size(),
									PROT_READ | PROT_WRITE,
									offset));
	}
}

void SharedFile::release()
{
	if (mFd >= 0)
	{
		close(mFd);
	}

	if (mBuffer)
	{
		munmap(mBuffer, mSize);

		LOG(mLog, DEBUG) << "Delete, fd: " << mFd;
	}
}

void SharedFile::createTmpFile()
{
	string templateName(getenv(cXdgRuntimeVar));

	if (templateName.empty())
	{
		throw Exception("Can't get XDG_RUNTIME_DIR environment var", EINVAL);
	}

	templateName += string(cFileNameTemplate);

	char name[templateName.length() + 1];

	strcpy(name, templateName.c_str());

	mFd = mkostemp(name, O_CLOEXEC);

	if (mFd < 0)
	{
		throw Exception("Can't create file: " + string(name), errno);
	}

	unlink(name);

	if (ftruncate(mFd, mSize) < 0)
	{
		throw Exception("Can't truncate file: " + string(name), errno);
	}

	LOG(mLog, DEBUG) << "Create tmp file: " << name;
}

}

