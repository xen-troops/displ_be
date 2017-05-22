/*
 * IlmControl.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_ILMCONTROL_HPP_
#define SRC_WAYLAND_ILMCONTROL_HPP_

#include <vector>
#include <memory>
#include <unordered_map>

#include <ilm/ilm_types.h>

#include <xen/be/Log.hpp>

namespace Wayland {

class IlmControl
{
public:

	IlmControl();
	~IlmControl();

	void addSurface(t_ilm_surface surfaceId, uint32_t screen,
					uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t z);
	void showSurface(t_ilm_surface surfaceId);
	void hideSurface(t_ilm_surface surfaceId);

public:

	struct SurfaceInfo
	{
		t_ilm_surface id;
		t_ilm_uint screen;
		t_ilm_uint x;
		t_ilm_uint y;
		t_ilm_uint w;
		t_ilm_uint h;
		t_ilm_uint z;
	};

	bool mNeedCommit;

	XenBackend::Log mLog;

	std::vector<SurfaceInfo> mSurfaces;
	std::vector<t_ilm_layer> mLayers;

	void showScreenInfo();
	void getScreenResolution(t_ilm_display screenId,
							 t_ilm_uint& width, t_ilm_uint& height);
	void setupLayers();
	t_ilm_layer createLayer(t_ilm_display screenId);
	void setLayerRenderOrder(t_ilm_layer layerId,
							 std::vector<t_ilm_surface>& surfaces);
	void setScreenRenderOrder(t_ilm_display screenId, t_ilm_layer layerId);
	void setLayerVisibility(t_ilm_layer layerId, bool visible);
	void destroyLayers();
	void setSurfaceVisibility(t_ilm_surface surfaceId, bool visible);
	void setSurfaceSourceRegion(t_ilm_surface surfaceId,
								t_ilm_uint x, t_ilm_uint y,
								t_ilm_uint w, t_ilm_uint h);
	void setSurfaceDestinationRegion(t_ilm_surface surfaceId,
									 t_ilm_uint x, t_ilm_uint y,
									 t_ilm_uint w, t_ilm_uint h);
	void commitChanges();
};

typedef std::shared_ptr<IlmControl> IlmControlPtr;

}

#endif /* SRC_WAYLAND_ILMCONTROL_HPP_ */
