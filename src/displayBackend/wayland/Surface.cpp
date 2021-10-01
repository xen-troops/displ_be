/*
 * Surface.cpp
 *
 *  Created on: Nov 24, 2016
 *      Author: al1
 */

#include <cassert>
#include "Surface.hpp"

#include "Exception.hpp"
#include "FrameBuffer.hpp"
#include "Compositor.hpp"

#include <sys/time.h>


using std::chrono::milliseconds;
using std::mutex;
using std::thread;
using std::unique_lock;

using DisplayItf::FrameBufferPtr;

namespace Wayland {

/*******************************************************************************
 * Surface
 ******************************************************************************/

Surface::Surface(Compositor* compositor) :
	mWlSurface(nullptr),
	mWlFrameCallback(nullptr),
	mBuffer(nullptr),
	mTerminate(false),
	mWaitForFrame(false),
	mLog("Surface"),
	mCompositor(compositor)
{
	assert(compositor != nullptr);
	try
	{
		init(mCompositor->getCompositor());
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}

#ifdef WITH_WAYLAND_PRESENTATION_API
	mWlPresentationFeedbackListener = {
		sPresentationFeedbackHandleSyncOutput, sPresentationFeedbackHandlePresented,
		sPresentationFeedbackHandleDiscarded
	};
#endif
}

Surface::~Surface()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void Surface::draw(FrameBufferPtr frameBuffer,
				   FrameCallback callback)
{
	unique_lock<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Draw";

	mStoredCallback = callback;

#ifndef WITH_WAYLAND_PRESENTATION_API
	if (mStoredCallback && !mWaitForFrame)
	{
		if (!mWlFrameCallback)
		{
			mWlFrameCallback = wl_surface_frame(mWlSurface);

			if (!mWlFrameCallback)
			{
				throw Exception("Can't get frame callback", errno);
			}

			if (wl_callback_add_listener(mWlFrameCallback,
					&mWlFrameListener, this) < 0)
			{
				throw Exception("Can't add listener", errno);
			}
		}
	}
#endif

	mBuffer = dynamic_cast<WlBuffer*>(frameBuffer.get());

	if (!mBuffer)
	{
		throw Exception("FrameBuffer must have type WlBuffer.", EINVAL);
	}

	mBuffer->setSurface(this);
	
	wl_surface_damage(mWlSurface, 0, 0,
					  mBuffer->getWidth(),
					  mBuffer->getHeight());

	wl_surface_attach(mWlSurface, mBuffer->getWLBuffer(), 0, 0);

	wl_surface_commit(mWlSurface);

#ifdef WITH_WAYLAND_PRESENTATION_API
	auto presentation = mCompositor->getPresentation();

	if (presentation)
	{
		mFeedback = wp_presentation_feedback(presentation, mWlSurface);
		wp_presentation_feedback_add_listener(mFeedback, &mWlPresentationFeedbackListener, this);
	}
#endif

	mCondVar.notify_one();

}

void Surface::clear()
{
	unique_lock<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Clear";

	mBuffer = nullptr;

	wl_surface_attach(mWlSurface, nullptr, 0, 0);

	wl_surface_commit(mWlSurface);
}

/*******************************************************************************
 * Private
 ******************************************************************************/
#ifdef WITH_WAYLAND_PRESENTATION_API
void Surface::sPresentationFeedbackHandleSyncOutput(void *data,
	struct wp_presentation_feedback *feedback, struct wl_output *output) {
	/* Not interested */
}
void Surface::sPresentationFeedbackHandlePresented(void *data,
		struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags) {
			static_cast<Surface*>(data)->framePresented(wp_feedback, tv_sec_hi, tv_sec_lo, tv_nsec, refresh_ns,
		seq_hi, seq_lo, flags);
}

void Surface::sPresentationFeedbackHandleDiscarded(void *data,
	struct wp_presentation_feedback *wp_feedback) {
}

void Surface::framePresented(struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags) {
	unique_lock<mutex> lock(mMutex);
	DLOG(mLog, DEBUG) << "Presentation feedback";
	sendCallback();

	if (mWaitForFrame)
	{
		mWaitForFrame = false;

		LOG(mLog, DEBUG) << "Surface is active";
	}
	else
	{
		mCondVar.notify_one();
	}
}
#else
void Surface::sFrameHandler(void *data, wl_callback *wl_callback,
							uint32_t callback_data)
{
	static_cast<Surface*>(data)->frameHandler();
}

void Surface::frameHandler()
{
	unique_lock<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Frame handler";

	wl_callback_destroy(mWlFrameCallback);

	mWlFrameCallback = nullptr;

	sendCallback();

	if (mWaitForFrame)
	{
		mWaitForFrame = false;

		LOG(mLog, DEBUG) << "Surface is active";
	}
	else
	{
		mCondVar.notify_one();
	}
}
#endif

void Surface::sendCallback()
{
	if (mStoredCallback)
	{
		mStoredCallback();

		mStoredCallback = nullptr;
	}
}

void Surface::run()
{
	unique_lock<mutex> lock(mMutex);

	while(!mTerminate)
	{
		if (!mStoredCallback)
		{
			mCondVar.wait(lock);
		}
		else
		{
			if (mWaitForFrame ||
				!mCondVar.wait_for(lock, milliseconds(cFrameTimeoutMs),
								   [this] { return !mStoredCallback; }))
			{
				if (!mWaitForFrame)
				{
					mWaitForFrame = true;

					LOG(mLog, DEBUG) << "Surface is inactive";
				}

				sendCallback();
			}
		}
	}
}

void Surface::stop()
{
	unique_lock<mutex> lock(mMutex);

	mTerminate = true;

	mCondVar.notify_one();
}

void Surface::init(wl_compositor* compositor)
{
	mWlSurface = wl_compositor_create_surface(compositor);

	if (!mWlSurface)
	{
		throw Exception("Can't create surface", errno);
	}

#ifndef WITH_WAYLAND_PRESENTATION_API
	mWlFrameListener = { sFrameHandler };
#endif

	mThread = thread(&Surface::run, this);

	LOG(mLog, DEBUG) << "Create: " << mWlSurface;
}

void Surface::release()
{
	if (mBuffer)
	{
		mBuffer->setSurface(nullptr);
	}

	clear();

	stop();

	if (mThread.joinable())
	{
		mThread.join();
	}

	if (mWlFrameCallback)
	{
		wl_callback_destroy(mWlFrameCallback);
	}

	if (mWlSurface)
	{
		wl_surface_destroy(mWlSurface);

		LOG(mLog, DEBUG) << "Delete: " << mWlSurface;
	}
}

}
