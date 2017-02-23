/*
 * IviSurface.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: al1
 */

#include "IviSurface.hpp"

#include <drm_fourcc.h>
#include <ilm/ilm_client.h>

#include "Exception.hpp"

using std::unordered_map;

namespace Wayland {

unordered_map<uint32_t, ilmPixelFormat> IviSurface::sPixelFormatMap =
{
	{DRM_FORMAT_RGB888, 	ILM_PIXELFORMAT_RGB_888},
	{DRM_FORMAT_RGBA8888, 	ILM_PIXELFORMAT_RGBA_8888},
	{DRM_FORMAT_RGB565,		ILM_PIXELFORMAT_RGB_565},
	{DRM_FORMAT_RGBA5551,	ILM_PIXELFORMAT_RGBA_5551},
	{DRM_FORMAT_RGBA4444,	ILM_PIXELFORMAT_RGBA_4444}
};

/*******************************************************************************
 * IviSurface
 ******************************************************************************/

IviSurface::IviSurface(SurfacePtr surface,
					   uint32_t width, uint32_t height, uint32_t pixelFormat) :
	mSurface(surface),
	mLog("IviSurface")
{
	try
	{
		init(width, height, pixelFormat);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

IviSurface::~IviSurface()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

/*******************************************************************************
 * Private
 ******************************************************************************/

void IviSurface::init(uint32_t width, uint32_t height, uint32_t pixelFormat)
{
	static t_ilm_surface sSurfaceId = 1000;

	mIlmSurface = sSurfaceId++;

	if (ilm_surfaceCreate(
			reinterpret_cast<t_ilm_nativehandle>(mSurface->mWlSurface),
			width, height, convertPixelFormat(pixelFormat), &mIlmSurface) !=
		ILM_SUCCESS)
	{
		throw Exception("Can't create ivi surface");
	}

	LOG(mLog, DEBUG) << "Create, surface id: " << mIlmSurface;
}

void IviSurface::release()
{
	if (mIlmSurface != INVALID_ID)
	{
		ilm_surfaceRemove(mIlmSurface);

		LOG(mLog, DEBUG) << "Delete";
	}
}

ilmPixelFormat IviSurface::convertPixelFormat(uint32_t pixelFormat)
{
	auto result = sPixelFormatMap.find(pixelFormat);

	if (result == sPixelFormatMap.end())
	{
		return ILM_PIXEL_FORMAT_UNKNOWN;
	}

	return result->second;
}

}
