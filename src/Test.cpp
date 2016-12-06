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

#include "drm/Device.hpp"
#include "wayland/Display.hpp"

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <iostream>

#include <sys/time.h>

#include <drm_fourcc.h>

using std::bind;
using std::cin;
using std::exception;
using std::shared_ptr;
using std::setprecision;
using std::fixed;
using std::string;
using std::thread;

#define WIDTH (1920/2)
#define HEIGHT 1080
#define BPP 32
#define BUFFER_SIZE (WIDTH*HEIGHT*BPP/4)

struct Rgb
{
	uint8_t x;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

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

void flipHandler(shared_ptr<ConnectorItf> connector,
			 shared_ptr<FrameBufferItf> frameBuffer1,
			 shared_ptr<FrameBufferItf> frameBuffer2)
{
	static int count = 0;

	while(!gTerminate)
	{
		LOG("Test", DEBUG) << "Flip handler";

		if (count % 2)
		{
			memcpy(frameBuffer1->getDisplayBuffer()->getBuffer(), gBuffer2, frameBuffer1->getDisplayBuffer()->getSize());
		}
		else
		{
			memcpy(frameBuffer1->getDisplayBuffer()->getBuffer(), gBuffer1, frameBuffer1->getDisplayBuffer()->getSize());
		}

		connector->pageFlip(frameBuffer1, flipDone);

		count++;

		if (count == 60)
		{
			static timeval start;
			timeval end;

			gettimeofday(&end, NULL);
			double t = end.tv_sec + end.tv_usec * 1e-6 - (start.tv_sec + start.tv_usec * 1e-6);

			LOG("Test", INFO) << "Freq: " << fixed << setprecision(2) << ((double)count / t);

			LOG("Test", INFO) << "Size: " << frameBuffer1->getDisplayBuffer()->getSize()
								<<  ", Q: " << (frameBuffer1->getDisplayBuffer()->getSize() % 4096);

			count = 0;
			start = end;
		}

		std::unique_lock<std::mutex> lock(gMutex);
		gCondVar.wait(lock);
	}

	LOG("Test", DEBUG) << "Finished";
}

int main(int argc, char *argv[])
{
	try
	{
		XenBackend::Log::setLogLevel("DEBUG");

		try
		{
//			Drm::Device display("/dev/dri/card0");
			Wayland::Display display;

			display.start();

			display.createConnector(37, 0, 0, WIDTH, HEIGHT);

			auto connector = display.getConnectorById(37);

			auto displayBuffer1 = display.createDisplayBuffer(WIDTH, HEIGHT, 32);

			auto frameBuffer1 = display.createFrameBuffer(displayBuffer1,
														  WIDTH, HEIGHT,
														  DRM_FORMAT_XRGB8888);

			auto displayBuffer2 = display.createDisplayBuffer(WIDTH, HEIGHT, 32);

			auto frameBuffer2 = display.createFrameBuffer(displayBuffer2,
														  WIDTH, HEIGHT,
														  DRM_FORMAT_XRGB8888);

			Rgb* data1 = reinterpret_cast<Rgb*>(gBuffer1);

			for (size_t i = 0; i < displayBuffer1->getSize() / sizeof(Rgb); i++)
			{
				data1[i].x = 0x00;
				data1[i].r = 0x00;
				data1[i].g = 0xFF;
				data1[i].b = 0x00;
			}

			Rgb* data2 = reinterpret_cast<Rgb*>(gBuffer2);

			for (size_t i = 0; i < displayBuffer1->getSize() / sizeof(Rgb); i++)
			{
				data2[i].x = 0x00;
				data2[i].r = 0xFF;
				data2[i].g = 0x00;
				data2[i].b = 0x00;
			}

			connector->init(0, 0, WIDTH, HEIGHT, frameBuffer1);


			gTerminate = false;

			thread flipThread(bind(flipHandler, connector, frameBuffer1, frameBuffer2));

			string str;
			cin >> str;

			{
				std::unique_lock<std::mutex> lock(gMutex);

				gTerminate = true;

				gCondVar.notify_one();
			}

			flipThread.join();

		}
		catch(const DisplayItfException& e)
		{
			LOG("Test", DEBUG) << e.what();
		}

	}
	catch(const exception& e)
	{
		LOG("Test", DEBUG) << e.what();
	}
	return 0;
}
