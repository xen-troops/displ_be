/*
 *  Test
 *
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
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <iostream>

#include <sys/time.h>
#include <csignal>
#include <execinfo.h>
#include <getopt.h>
#include <unistd.h>

#include <drm_fourcc.h>

#include "drm/Display.hpp"
#include "wayland/Display.hpp"
#include "input/InputManager.hpp"
#include "input/WlInput.hpp"

using std::bind;
using std::cin;
using std::dynamic_pointer_cast;
using std::make_shared;
using std::setprecision;
using std::fixed;
using std::string;
using std::thread;

using DisplayItf::ConnectorPtr;
using DisplayItf::DisplayBufferPtr;
using DisplayItf::FrameBufferPtr;

#define BACK_WIDTH 640
#define BACK_HEIGHT 480

#define WIDTH (BACK_WIDTH/2)
#define HEIGHT BACK_HEIGHT
#define BPP 32
#define BUFFER_SIZE (WIDTH*HEIGHT*BPP/4)

/// @cond HIDDEN_SYMBOLS
struct Rgb
{
	uint8_t x;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
/// @endcond


std::atomic_bool gTerminate;
std::condition_variable gCondVar;
std::mutex gMutex;

uint8_t gBuffer1[BUFFER_SIZE];
uint8_t gBuffer2[BUFFER_SIZE];

void flipDone()
{
	LOG("Test", DEBUG) << "Flip done";

	std::unique_lock<std::mutex> lock(gMutex);
	gCondVar.notify_one();
}

void flipHandler(ConnectorPtr connector,
				 FrameBufferPtr frameBuffer1,
				 FrameBufferPtr frameBuffer2)
{
	static int count = 0;

	while(!gTerminate)
	{
		LOG("Test", DEBUG) << "Flip handler";

		if (count % 2)
		{
			memcpy(frameBuffer1->getDisplayBuffer()->getBuffer(), gBuffer2,
				   frameBuffer1->getDisplayBuffer()->getSize());
		}
		else
		{
			memcpy(frameBuffer1->getDisplayBuffer()->getBuffer(), gBuffer1,
				   frameBuffer1->getDisplayBuffer()->getSize());
		}

		connector->pageFlip(frameBuffer1, flipDone);

		count++;

		if (count == 60)
		{
			static timeval start;
			timeval end;

			gettimeofday(&end, NULL);
			double t = end.tv_sec + end.tv_usec * 1e-6 -
					   (start.tv_sec + start.tv_usec * 1e-6);

			LOG("Test", INFO) << "Freq: " << fixed << setprecision(2)
							  << ((double)count / t);

			LOG("Test", INFO) << "Size: "
							  << frameBuffer1->getDisplayBuffer()->getSize()
							  <<  ", Q: "
							  << (frameBuffer1->getDisplayBuffer()->getSize() %
								  4096);

			count = 0;
			start = end;
		}

		std::unique_lock<std::mutex> lock(gMutex);
		gCondVar.wait(lock);
	}

	LOG("Test", DEBUG) << "Finished";
}

void pointerMove1(int32_t x, int32_t y)
{
	LOG("Move1", DEBUG) << "X: " << x << ", Y: " << y;
}

void pointerMove2(int32_t x, int32_t y)
{
	LOG("Move2", DEBUG) << "X: " << x << ", Y: " << y;
}

void keyboardEvent1(uint32_t key, uint32_t state)
{
	LOG("Key1", DEBUG) << "key: " << key << ", state: " << state;
}

void keyboardEvent2(uint32_t key, uint32_t state)
{
	LOG("Key2", DEBUG) << "key: " << key << ", state: " << state;
}

void segmentationHandler(int sig)
{
	void *array[20];
	size_t size;

	LOG("Main", ERROR) << "Segmentation fault!";

	size = backtrace(array, 20);

	backtrace_symbols_fd(array, size, STDERR_FILENO);

	exit(1);
}

void registerSignals()
{
//	signal(SIGINT, terminateHandler);
//	signal(SIGTERM, terminateHandler);
	signal(SIGSEGV, segmentationHandler);
}

void waitSignals()
{
	sigset_t set;
	int signal;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigprocmask(SIG_BLOCK, &set, nullptr);

	sigwait(&set,&signal);
}

int main(int argc, char *argv[])
{
	try
	{
		XenBackend::Log::setLogLevel("DEBUG");

		registerSignals();

		Drm::Display display("/dev/dri/card0");

		display.autoCreateConnectors();

		auto connector1 = display.getConnectorById(0);


//		Wayland::Display display;
//		display.createBackgroundSurface(BACK_WIDTH, BACK_HEIGHT);

//		auto connector1 = display.createConnector(37, 0, 0, WIDTH, HEIGHT);
//		auto connector2 = display.createConnector(38, WIDTH, 0, WIDTH, HEIGHT);


		display.start();

		auto displayBuffer1 = display.createDisplayBuffer(WIDTH, HEIGHT, 32);
		auto frameBuffer1 = display.createFrameBuffer(displayBuffer1,
													  WIDTH, HEIGHT,
													  DRM_FORMAT_XRGB8888);

		auto displayBuffer2 = display.createDisplayBuffer(WIDTH, HEIGHT, 32);
		auto frameBuffer2 = display.createFrameBuffer(displayBuffer2,
													  WIDTH, HEIGHT,
													  DRM_FORMAT_XRGB8888);

		Rgb* data1 = reinterpret_cast<Rgb*>(gBuffer1);
//		Rgb* data1 = reinterpret_cast<Rgb*>(displayBuffer1->getBuffer());

		for (size_t i = 0; i < displayBuffer1->getSize() / sizeof(Rgb); i++)
		{
			data1[i].x = 0x00;
			data1[i].r = 0x00;
			data1[i].g = 0xFF;
			data1[i].b = 0x00;
		}

		Rgb* data2 = reinterpret_cast<Rgb*>(gBuffer2);
//		Rgb* data2 = reinterpret_cast<Rgb*>(displayBuffer2->getBuffer());

		for (size_t i = 0; i < displayBuffer1->getSize() / sizeof(Rgb); i++)
		{
			data2[i].x = 0x00;
			data2[i].r = 0xFF;
			data2[i].g = 0x00;
			data2[i].b = 0xFF;
		}

		connector1->init(0, 0, WIDTH, HEIGHT, frameBuffer1);
		// connector2->init(0, 0, WIDTH, HEIGHT, frameBuffer2);

		gTerminate = false;

		thread flipThread(bind(flipHandler, connector1,
							   frameBuffer1, frameBuffer2));

#if 0
		Input::WlKeyboard keyboard1(display, 37);
		Input::WlKeyboard keyboard2(display, 38);

		Input::WlPointer pointer1(display, 37);
		Input::WlPointer pointer2(display, 38);

		Input::WlTouch touch1(display, 37);
		Input::WlTouch touch2(display, 38);

		pointer1.setCallbacks({pointerMove1});
		pointer2.setCallbacks({pointerMove2});

		keyboard1.setCallbacks({keyboardEvent1});
		keyboard2.setCallbacks({keyboardEvent2});


		Input::InputManager inputManager;

		inputManager.createInputKeyboard(0, "/dev/input/event7");
#endif

		waitSignals();

		gTerminate = true;

		flipThread.join();

	}
	catch(const std::exception& e)
	{
		LOG("Test", ERROR) << e.what();
	}
	return 0;
}
