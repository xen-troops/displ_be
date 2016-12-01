/*
 *  WlTest
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

#include "wayland/Display.hpp"

#include <iostream>

using std::exception;
using std::string;
using std::cin;

using Wayland::Display;

void flipped()
{
	LOG("Main", DEBUG) << "Flipped";
}

int main(int argc, char *argv[])
{
	try
	{
		XenBackend::Log::setLogLevel("DEBUG");

		Display display;

		display.start();

		display.createConnector(0, 0, 0, 800, 600);

		auto connector = display.getConnectorById(0);

//		display.createConnector(2, 640, 0, 640, 800);

		auto displayBuffer1 = display.createDisplayBuffer(800, 600, 32);

		auto frameBuffer1 = display.createFrameBuffer(displayBuffer1, 800, 600,
													  WL_SHM_FORMAT_XRGB8888);


		connector->init(0, 0, 800, 600, frameBuffer1);

		auto displayBuffer2 = display.createDisplayBuffer(800, 600, 32);

		auto frameBuffer2 = display.createFrameBuffer(displayBuffer2, 800, 600,
													  WL_SHM_FORMAT_XRGB8888);

#if 0
		struct Rgb
		{
			uint8_t x;
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};

		Rgb* data1 = static_cast<Rgb*>(displayBuffer1->getBuffer());
		Rgb* data2 = static_cast<Rgb*>(displayBuffer2->getBuffer());

		for (size_t i = 0; i < displayBuffer1->getSize() / sizeof(Rgb); i++)
		{
			data1[i].x = 0x00;
			data1[i].r = 0x00;
			data1[i].g = 0xFF;
			data1[i].b = 0x00;
		}

		for (size_t i = 0; i < displayBuffer2->getSize() / sizeof(Rgb); i++)
		{
			data2[i].x = 0x00;
			data2[i].r = 0xFF;
			data2[i].g = 0x00;
			data2[i].b = 0x00;
		}
#endif

	//	display.getConnectorById(1)->init(0, 0, 800, 600, frameBuffer1);
	//	display.getConnectorById(2)->init(0, 0, 640, 800, frameBuffer2);

#if 0
		gSurface1 = display.getCompositor()->createSurface();
		gSurface2 = display.getCompositor()->createSurface();

		auto shellSurface1 = display.getShell()->getShellSurface(gSurface1);
		auto shellSurface2 = display.getShell()->getShellSurface(gSurface2);

		shellSurface1->setTopLevel();
		shellSurface2->setTopLevel();

		gSharedFile1 = display.getSharedMemory()->
				createSharedFile(320, 240, 32);

		gSharedBuffer1 = display.getSharedMemory()->
								  createSharedBuffer(gSharedFile1, 320, 240,
										  gSharedFile1->getStride(),
								  WL_SHM_FORMAT_XRGB8888);

		gSharedFile2 = display.getSharedMemory()->
				createSharedFile(320, 240, 32);

		gSharedBuffer2 = display.getSharedMemory()->
								  createSharedBuffer(gSharedFile2, 320, 240,
										  gSharedFile2->getStride(),
								  WL_SHM_FORMAT_XRGB8888);

		LOG("Main", DEBUG) << "Buffer size: " << gSharedFile1->getSize();

		struct Rgb
		{
			uint8_t x;
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};

		Rgb* data1 = static_cast<Rgb*>(gSharedFile1->getBuffer());
		Rgb* data2 = static_cast<Rgb*>(gSharedFile2->getBuffer());

		for (size_t i = 0; i < gSharedFile1->getSize() / sizeof(Rgb); i++)
		{
			data1[i].x = 0x00;
			data1[i].r = 0x00;
			data1[i].g = 0xFF;
			data1[i].b = 0x00;
		}

		for (size_t i = 0; i < gSharedFile2->getSize() / sizeof(Rgb); i++)
		{
			data2[i].x = 0x00;
			data2[i].r = 0xFF;
			data2[i].g = 0x00;
			data2[i].b = 0x00;
		}

//		drawFrame(gSharedFile1, gSharedBuffer1, gSharedFile2, gSharedBuffer2);

		gSurface1->draw(gSharedBuffer1);
		gSurface2->draw(gSharedBuffer2);

#endif

		try
		{
			connector->pageFlip(frameBuffer2, flipped);
//			display.getConnectorById(2)->pageFlip(frameBuffer2, flipped);
		}
		catch(const DisplayItfException& e)
		{
			LOG("Main", DEBUG) << e.what();
		}

		string str;
		cin >> str;
	}
	catch(const exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}
	return 0;
}
