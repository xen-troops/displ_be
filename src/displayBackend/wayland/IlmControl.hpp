/*
 * IlmControl.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: al1
 */

#ifndef SRC_WAYLAND_ILMCONTROL_HPP_
#define SRC_WAYLAND_ILMCONTROL_HPP_

#include <memory>

#include <xen/be/Log.hpp>

namespace Wayland {

class IlmControl
{
public:

	IlmControl();
	~IlmControl();

	void commitChanges();

public:

	XenBackend::Log mLog;

	void showScreenInfo();
};

typedef std::shared_ptr<IlmControl> IlmControlPtr;

}

#endif /* SRC_WAYLAND_ILMCONTROL_HPP_ */
