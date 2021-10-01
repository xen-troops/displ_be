/*
 * Surface.hpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#ifndef SRC_WAYLAND_SURFACE_HPP_
#define SRC_WAYLAND_SURFACE_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>

#include <wayland-client.h>

#include <xen/be/Log.hpp>

#include <sys/time.h>

#ifdef WITH_WAYLAND_PRESENTATION_API
#include "presentation-time-client-protocol.h"
#endif

#include "DisplayItf.hpp"

namespace Wayland {

class WlBuffer;
class Compositor;

/***************************************************************************//**
 * Wayland surface class.
 * @ingroup wayland
 ******************************************************************************/
class Surface
{
public:

	/**
	 * Callback which is called when frame is displayed
	 */
	typedef std::function<void()> FrameCallback;

	~Surface();

	/**
	 * Draws shared buffer content
	 * @param frameBuffer  shared buffer
	 * @param callback     called when frame with the content is displayed
	 */
	void draw(DisplayItf::FrameBufferPtr frameBuffer,
			  FrameCallback callback = nullptr);

	/**
	 * Clear surface
	 */
	void clear();

private:

	friend class Display;
	friend class IviSurface;
	template <typename T> friend class SeatDevice;
	friend class ShellSurface;
	friend class Compositor;
	friend class Connector;

	const uint32_t cFrameTimeoutMs = 50;

	explicit Surface(Compositor* compositor);

	wl_surface* mWlSurface;
	wl_callback *mWlFrameCallback;
	WlBuffer* mBuffer;
	bool mTerminate;
	bool mWaitForFrame;
	XenBackend::Log mLog;

	std::mutex mMutex;
	std::condition_variable mCondVar;
	std::thread mThread;

#ifdef WITH_WAYLAND_PRESENTATION_API
	wp_presentation *mWlPresentation{nullptr};
	wp_presentation_feedback_listener mWlPresentationFeedbackListener;
	struct wp_presentation_feedback* mFeedback{nullptr};
#else
	wl_callback_listener mWlFrameListener;
#endif

	Compositor* mCompositor{nullptr};
	FrameCallback mStoredCallback;

#ifdef WITH_WAYLAND_PRESENTATION_API
	static void sPresentationFeedbackHandleSyncOutput(void *data,
		struct wp_presentation_feedback *feedback, struct wl_output *output);

	static void sPresentationFeedbackHandlePresented(void *data,
		struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags);

	static void sPresentationFeedbackHandleDiscarded(void *data,
		struct wp_presentation_feedback *wp_feedback);

	void framePresented(struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags);
#else
	static void sFrameHandler(void *data, wl_callback *wl_callback,
							  uint32_t callback_data);
	void frameHandler();
#endif

	void sendCallback();

	void run();
	void stop();

	void init(wl_compositor* compositor);
	void release();
};

typedef std::shared_ptr<Surface> SurfacePtr;

}

#endif /* SRC_WAYLAND_SURFACE_HPP_ */
