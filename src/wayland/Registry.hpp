/*
 * Registry.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_REGISTRY_HPP_
#define SRC_WAYLAND_REGISTRY_HPP_

#include <wayland-client.h>

namespace Wayland {

class Registry
{
public:
	uint32_t getId() const { return mId; }

protected:

	Registry(wl_registry* registry, uint32_t id, uint32_t version) :
		mRegistry(registry), mId(id), mVersion(version) {}

	wl_registry* getRegistry() const { return mRegistry; }
	uint32_t getVersion() const { return mVersion; }

private:
	wl_registry* mRegistry;
	uint32_t mId;
	uint32_t mVersion;
};

}

#endif /* SRC_WAYLAND_REGISTRY_HPP_ */
