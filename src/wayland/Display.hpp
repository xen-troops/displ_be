/*
 * Display.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_DISPLAY_HPP_
#define SRC_WAYLAND_DISPLAY_HPP_

#include <atomic>
#include <memory>
#include <thread>

#include <xen/be/Log.hpp>

#include "Compositor.hpp"
#include "SharedMemory.hpp"
#include "Shell.hpp"

namespace Wayland {

class Display
{
public:

	Display();
	~Display();

	void start();
	void stop();

	std::shared_ptr<Compositor> getCompositor() const;
	std::shared_ptr<Shell> getShell() const;
	std::shared_ptr<SharedMemory> getSharedMemory() const;

	void dispatch();

private:
	const int cPoolEventTimeoutMs = 100;

	wl_display* mDisplay;
	wl_registry* mRegistry;
	wl_registry_listener mRegistryListener;
	std::atomic_bool mTerminate;
	XenBackend::Log mLog;

	std::shared_ptr<Compositor> mCompositor;
	std::shared_ptr<Shell> mShell;
	std::shared_ptr<SharedMemory> mSharedMemory;

	std::thread mThread;

	static void sRegistryHandler(void *data, wl_registry *registry,
								 uint32_t id, const char *interface,
								 uint32_t version);
	static void sRegistryRemover(void *data, struct wl_registry *registry,
								 uint32_t id);

	void registryHandler(wl_registry *registry, uint32_t id,
						 const std::string& interface, uint32_t version);
	void registryRemover(wl_registry *registry, uint32_t id);

	void init();
	void release();

	bool pollDisplayFd();
	void dispatchThread();
};

}

#endif /* SRC_WAYLAND_DISPLAY_HPP_ */
