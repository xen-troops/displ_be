/*
 * IlmControl.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: al1
 */

#include "IlmControl.hpp"

#include <algorithm>

#include <ilm/ilm_common.h>
#include <ilm/ilm_control.h>

#include "Exception.hpp"

using std::find_if;
using std::sort;
using std::string;
using std::stringstream;
using std::to_string;
using std::unordered_map;
using std::vector;

namespace Wayland {

/*******************************************************************************
 * IlmControl
 ******************************************************************************/

IlmControl::IlmControl() :
	mNeedCommit(false),
	mLog("IlmControl")
{
	auto result = ilm_init();

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't initialize ILM: " +
						string(ILM_ERROR_STRING(result)));
	}

	LOG(mLog, DEBUG) << "Init";

	showScreenInfo();
}

IlmControl::~IlmControl()
{
	destroyLayers();

	ilm_destroy();

	LOG(mLog, DEBUG) << "Destroy";
}

/*******************************************************************************
 * Public
 ******************************************************************************/
void IlmControl::addSurface(t_ilm_surface surfaceId, uint32_t screen,
							uint32_t x, uint32_t y, uint32_t w, uint32_t h,
							uint32_t z)
{
	SurfaceInfo info = {surfaceId, screen, x, y, w, h, z};

	mSurfaces.push_back(info);

	mNeedCommit = true;

	LOG(mLog, DEBUG) << "Add surface id: " << surfaceId
					 << ", screen: " << screen
					 << ", x: " << x << ", y: " << y
					 << ", w: " << w << ", h: " << h
					 << ", z: " << z;
}

void IlmControl::showSurface(t_ilm_surface surfaceId)
{
	if (mNeedCommit)
	{
		setupLayers();

		mNeedCommit = false;
	}

	setSurfaceVisibility(surfaceId, true);

	commitChanges();
}

void IlmControl::hideSurface(t_ilm_surface surfaceId)
{
	setSurfaceVisibility(surfaceId, false);

	commitChanges();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void IlmControl::showScreenInfo()
{
	t_ilm_uint count = 0;
	t_ilm_display* ids = NULL;

	auto result = ilm_getScreenIDs(&count, &ids);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't get screen IDs: " +
						string(ILM_ERROR_STRING(result)));
	}

	for(unsigned int i = 0; i < count; i++)
	{
		t_ilm_uint width, height;

		getScreenResolution(ids[i], width, height);

		LOG(mLog, DEBUG) << "Screen id: " << ids[i]
						 << ", w: " << width << ", h: " << height;
	}
}

void IlmControl::getScreenResolution(t_ilm_display screenId,
									 t_ilm_uint& width, t_ilm_uint& height)
{
	auto result = ilm_getScreenResolution(screenId, &width, &height);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't get screen resolution: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::setupLayers()
{
	destroyLayers();

	// sort surfaces by Z order
	sort(mSurfaces.begin(), mSurfaces.end(),
		 [](const SurfaceInfo& a, const SurfaceInfo& b)
		 { return a.z > b.z; });

	unordered_map<t_ilm_display, vector<t_ilm_surface>> layers;

	// create map of layers for different screens
	// one layer per screen
	for(auto surface : mSurfaces)
	{
		layers[surface.screen].push_back(surface.id);
	}

	for(auto layer : layers)
	{
		LOG(mLog, DEBUG) << "Create layer for screen id: " << layer.first;

		auto layerId = createLayer(layer.first);

		mLayers.push_back(layerId);

		setLayerRenderOrder(layerId, layer.second);
		setScreenRenderOrder(layer.first, layerId);
		setLayerVisibility(layerId, true);
	}

	for (auto surface : mSurfaces)
	{
		setSurfaceSourceRegion(surface.id, 0, 0, surface.w, surface.h);
		setSurfaceDestinationRegion(surface.id, surface.x, surface.y,
									surface.w, surface.h);
	}

	commitChanges();
}

t_ilm_layer IlmControl::createLayer(t_ilm_display screenId)
{
	static t_ilm_layer sLayerId = 1000;

	t_ilm_uint width, height;

	getScreenResolution(screenId, width, height);

	LOG(mLog, DEBUG) << "Create layer for screen: " << screenId
					 << ", layer ID: " << sLayerId
					 << ", w: " << width << ", h: " << height;

	auto result = ilm_layerCreateWithDimension(&sLayerId, width, height);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't create layer: " +
						string(ILM_ERROR_STRING(result)));
	}

	return sLayerId++;
}

void IlmControl::setLayerRenderOrder(t_ilm_layer layerId,
									 vector<t_ilm_surface>& surfaces)
{
	stringstream ss;

	for(auto id : surfaces)
	{
		ss << id << " ";
	}

	LOG(mLog, DEBUG) << "Set layer render order, layer id: " << layerId
					 << ", surface: " << ss.str();

	auto result = ilm_layerSetRenderOrder(layerId, surfaces.data(),
										  surfaces.size());

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set layer render order: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::setScreenRenderOrder(t_ilm_display screenId,
									  t_ilm_layer layerId)
{
	LOG(mLog, DEBUG) << "Set screen render order, screen id: " << screenId
					 << ", layer id: " << layerId;

	auto result = ilm_displaySetRenderOrder(screenId, &layerId, 1);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set screen render order: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::setLayerVisibility(t_ilm_layer layerId, bool visible)
{
	LOG(mLog, DEBUG) << "Set layer visibility, layer id: " << layerId
					 << ", visible: " << visible;

	auto result = ilm_layerSetVisibility(layerId, visible);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set layer visibility: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::destroyLayers()
{
	for(auto layerId : mLayers)
	{
		LOG(mLog, DEBUG) << "Destroy layer: " << layerId;

		ilm_layerRemove(layerId);
	}
}

void IlmControl::setSurfaceVisibility(t_ilm_surface surfaceId, bool visible)
{
	LOG(mLog, DEBUG) << "Set surface visibility, surface id: " << surfaceId
					 << ", visible: " << visible;

	auto result = ilm_surfaceSetVisibility(surfaceId, visible);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set surface visibility: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::setSurfaceSourceRegion(t_ilm_surface surfaceId,
										t_ilm_uint x, t_ilm_uint y,
										t_ilm_uint w, t_ilm_uint h)
{
	LOG(mLog, DEBUG) << "Set surface source region, surface id: " << surfaceId
					 << ", x: " << x << ", y: " << y
					 << ", w: " << w << ", h: " << h;

	auto result = ilm_surfaceSetSourceRectangle(surfaceId, x, y, w, h);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set surface source region: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::setSurfaceDestinationRegion(t_ilm_surface surfaceId,
											 t_ilm_uint x, t_ilm_uint y,
											 t_ilm_uint w, t_ilm_uint h)
{
	LOG(mLog, DEBUG) << "Set surface destination region, surface id: "
					 << surfaceId
					 << ", x: " << x << ", y: " << y
					 << ", w: " << w << ", h: " << h;

	auto result = ilm_surfaceSetDestinationRectangle(surfaceId, x, y, w, h);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't set surface destination region: " +
						string(ILM_ERROR_STRING(result)));
	}
}

void IlmControl::commitChanges()
{
	auto result = ilm_commitChanges();

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't commit changes: " +
						string(ILM_ERROR_STRING(result)));
	}
}

}
