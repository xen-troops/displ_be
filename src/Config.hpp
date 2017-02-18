/*
 * Config.hpp
 *
 *  Created on: Feb 17, 2017
 *      Author: al1
 */

#ifndef SRC_CONFIG_HPP_
#define SRC_CONFIG_HPP_

#include <string>

#include <libconfig.h++>

#include <xen/be/Log.hpp>

class Config
{
public:

	Config(const std::string& fileName);

private:

	XenBackend::Log mLog;
	libconfig::Config mConfig;
};

#endif /* SRC_CONFIG_HPP_ */
