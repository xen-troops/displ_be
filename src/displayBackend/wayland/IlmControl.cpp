/*
 * IlmControl.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: al1
 */

#include "IlmControl.hpp"

#include <ilm/ilm_common.h>
#include <ilm/ilm_control.h>

#include "Exception.hpp"

using std::string;

namespace Wayland {

/*******************************************************************************
 * IlmControl
 ******************************************************************************/

IlmControl::IlmControl() :
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
	ilm_destroy();

	LOG(mLog, DEBUG) << "Destroy";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void IlmControl::commitChanges()
{

}

/*******************************************************************************
 * Private
 ******************************************************************************/

void IlmControl::showScreenInfo()
{
	unsigned int count = 0;
	unsigned int* ids = NULL;

	auto result = ilm_getScreenIDs(&count, &ids);

	if (result != ILM_SUCCESS)
	{
		throw Exception("Can't get screen IDs: " +
						string(ILM_ERROR_STRING(result)));
	}

	for(unsigned int i = 0; i < count; i++)
	{
		ilmScreenProperties props;

		result = ilm_getPropertiesOfScreen(ids[i], &props);

		if (result != ILM_SUCCESS)
		{
			throw Exception("Can't get screen properties: " +
							string(ILM_ERROR_STRING(result)));
		}

		LOG(mLog, DEBUG) << "Screen id: " << ids[i]
						 << ", w: " << props.screenWidth
						 << ", h: " << props.screenHeight
						 << ", HW layers: " << props.harwareLayerCount;
	}
}

}
