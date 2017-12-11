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

/***************************************************************************//**
 * Base class for Wayland registry.
 * @ingroup wayland
 ******************************************************************************/
class Registry
{
protected:

	/**
	 * @param registry wl registry structure
	 * @param id       registry id
	 * @param version  registry version
	 */
	Registry(wl_registry* registry, uint32_t id, uint32_t version) :
		mWlRegistry(registry), mId(id), mVersion(version) {}

	virtual ~Registry() {}

	/**
	 * Returns registry id
	 */
	uint32_t getId() const { return mId; }

	/**
	 * Returns wl registry structure
	 */
	wl_registry* getRegistry() const { return mWlRegistry; }

	/**
	 * Returns registry version
	 */
	uint32_t getVersion() const { return mVersion; }

	/**
	 * Binds registry
	 */
	void* bind(const wl_interface *interface)
	{
		return wl_registry_bind(mWlRegistry, mId, interface, mVersion);
	}

private:

	wl_registry* mWlRegistry;
	uint32_t mId;
	uint32_t mVersion;
};

}

#endif /* SRC_WAYLAND_REGISTRY_HPP_ */
