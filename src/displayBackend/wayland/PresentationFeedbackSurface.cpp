/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2022 EPAM Systems Inc.
 *
 */

#include <cassert>

#include "PresentationFeedbackSurface.hpp"
#include "FrameBuffer.hpp"
#include "Exception.hpp"
#include "Compositor.hpp"

using std::mutex;
using std::thread;
using std::unique_lock;

using DisplayItf::FrameBufferPtr;

namespace Wayland {

PresentationFeedbackSurface::PresentationFeedbackSurface(wl_compositor* compositor, wp_presentation* presentation)
: Surface(compositor), mWlPresentation(presentation)
{
	assert(mWlPresentation);
	mWlPresentationFeedbackListener = {
		sPresentationFeedbackHandleSyncOutput, sPresentationFeedbackHandlePresented,
		sPresentationFeedbackHandleDiscarded
	};
	mLog = std::string("PresentationFeedbackSurface");
}

PresentationFeedbackSurface::~PresentationFeedbackSurface()
{
}

void PresentationFeedbackSurface::disableCallback()
{
	unique_lock<mutex> lock(mMutex);

	mStoredCallback = nullptr;
	wp_presentation_destroy(mWlPresentation);
}

void PresentationFeedbackSurface::draw(FrameBufferPtr frameBuffer,
				   FrameCallback callback)
{
	unique_lock<mutex> lock(mMutex);

	DLOG(mLog, DEBUG) << "Draw";

	mStoredCallback = callback;
	mBuffer = dynamic_cast<WlBuffer*>(frameBuffer.get());

	if (!mBuffer)
	{
		throw Exception("FrameBuffer must have type WlBuffer.", EINVAL);
	}

	wl_surface_damage(mWlSurface, 0, 0,
					  mBuffer->getWidth(),
					  mBuffer->getHeight());
	wl_surface_attach(mWlSurface, mBuffer->getWLBuffer(), 0, 0);
	wl_surface_commit(mWlSurface);

	mFeedback = wp_presentation_feedback(mWlPresentation, mWlSurface);
	wp_presentation_feedback_add_listener(mFeedback, &mWlPresentationFeedbackListener, this);

	mCondVar.notify_one();
}

void PresentationFeedbackSurface::sPresentationFeedbackHandleSyncOutput(void *data,
	struct wp_presentation_feedback *feedback, struct wl_output *output) {
	/* Not interested */
}
void PresentationFeedbackSurface::sPresentationFeedbackHandlePresented(void *data,
		struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags) {
			static_cast<PresentationFeedbackSurface*>(data)->framePresented(wp_feedback, tv_sec_hi, tv_sec_lo, tv_nsec, refresh_ns,
		seq_hi, seq_lo, flags);
}

void PresentationFeedbackSurface::sPresentationFeedbackHandleDiscarded(void *data,
	struct wp_presentation_feedback *wp_feedback) {
}

void PresentationFeedbackSurface::framePresented(struct wp_presentation_feedback *wp_feedback, uint32_t tv_sec_hi,
		uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh_ns,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags) {
	unique_lock<mutex> lock(mMutex);

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

}
