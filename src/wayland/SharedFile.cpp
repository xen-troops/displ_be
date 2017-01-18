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

namespace Wayland {

/*******************************************************************************
 * SharedFile
 ******************************************************************************/

SharedFile::SharedFile(uint32_t width, uint32_t height, uint32_t bpp) :
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
		init();
	}
	catch(const WlException& e)
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
		throw WlException("There is no buffer to copy from");
	}

	DLOG("Dumb", DEBUG) << "Copy dumb, handle: " << mFd;

	memcpy(mBuffer, mGnttabBuffer->get(), mSize);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void SharedFile::init()
{
	createTmpFile();

	auto map = mmap(NULL, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);

	if (map == MAP_FAILED)
	{
		throw WlException("Can't map shared file");
	}

	mBuffer = map;

	LOG(mLog, DEBUG) << "Create, w: " << mWidth << ", h: " << mHeight
					 << ", stride: " << mStride << ", fd: " << mFd;
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
	const char* path(getenv(cXdgRuntimeVar));

	if (!path)
	{
		throw WlException("Can't get XDG_RUNTIME_DIR environment var");
	}

	char name[strlen(path) + strlen(cFileNameTemplate)];

	strcpy(name, path);
	strcat(name, cFileNameTemplate);

	mFd = mkostemp(name, O_CLOEXEC);

	if (mFd < 0)
	{
		throw WlException("Can't create file: " + string(name));
	}

	unlink(name);

	if (ftruncate(mFd, mSize) < 0)
	{
		throw WlException("Can't truncate file: " + string(name));
	}

	LOG(mLog, DEBUG) << "Create tmp file: " << name;
}

}

