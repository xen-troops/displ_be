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

#pragma once

#include "Surface.hpp"
#include "Registry.hpp"

#include "presentation-time-client-protocol.h"


namespace Wayland {

class PresentationFeedbackSurface : public Surface {
public:

	explicit PresentationFeedbackSurface(wl_compositor* compositor, wp_presentation* presentation);
	virtual ~PresentationFeedbackSurface();

	/**
	 * Draws shared buffer content
	 * @param frameBuffer  shared buffer
	 * @param callback     called when frame with the content is displayed
	 */
	virtual void draw(DisplayItf::FrameBufferPtr frameBuffer,
			  FrameCallback callback = nullptr);

	/**
	 * Disable callback from Wayland
	 */
	void disableCallback() override;

private:

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

	wp_presentation *mWlPresentation{nullptr};
	wp_presentation_feedback_listener mWlPresentationFeedbackListener;
	struct wp_presentation_feedback* mFeedback{nullptr};
 };

}
