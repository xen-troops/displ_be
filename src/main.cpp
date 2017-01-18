/*
 *  Backend
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

#include <algorithm>
#include <exception>
#include <iostream>

#include <csignal>
#include <execinfo.h>
#include <getopt.h>

#include <xen/be/Log.hpp>

#include "drm/Display.hpp"
#include "DisplayBackend.hpp"
#include "input/InputManager.hpp"
#include "InputBackend.hpp"

using std::chrono::milliseconds;
using std::atomic_bool;
using std::cout;
using std::endl;
using std::shared_ptr;
using std::string;
using std::this_thread::sleep_for;
using std::toupper;
using std::transform;

using XenBackend::Log;

DisplayMode gDisplayMode = DisplayMode::WAYLAND;

const uint32_t cWlBackgroundWidth = 1920;
const uint32_t cWlBackgroundHeight = 1080;

/*******************************************************************************
 *
 ******************************************************************************/

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

bool commandLineOptions(int argc, char *argv[])
{

	int opt = -1;

	while((opt = getopt(argc, argv, "m:v:fh?")) != -1)
	{
		switch(opt)
		{
		case 'v':
			if (!Log::setLogLevel(string(optarg)))
			{
				return false;
			}

			break;

		case 'm':
		{
			string strMode(optarg);

			transform(strMode.begin(), strMode.end(), strMode.begin(),
					  (int (*)(int))toupper);

			if (strMode == "DRM")
			{
				gDisplayMode = DisplayMode::DRM;
			}

			break;
		}
		case 'f':
			Log::setShowFileAndLine(true);
			break;

		default:
			return false;
		}
	}

	return true;
}

shared_ptr<Drm::Display> getDrmDisplay()
{
	shared_ptr<Drm::Display> device(new Drm::Display("/dev/dri/card0"));

	device->createConnector(0, 37);
	device->createConnector(1, 42);

	return device;
}

shared_ptr<Wayland::Display> getWaylandDisplay()
{
	shared_ptr<Wayland::Display> display(new Wayland::Display());

	display->createBackgroundSurface(cWlBackgroundWidth,
									 cWlBackgroundHeight);

	display->createConnector(0, 0, 0, cWlBackgroundWidth/2, cWlBackgroundHeight);
	display->createConnector(1, cWlBackgroundWidth/2, 0,
							 cWlBackgroundWidth/2, cWlBackgroundHeight);

	return display;
}

Input::InputManagerPtr getWlInputManager(shared_ptr<Wayland::Display> display)
{
	Input::InputManagerPtr inputManager(new Input::InputManager(display));

	inputManager->createWlKeyboard(0, 0);
	inputManager->createWlPointer(0, 0);
//	inputManager->createWlTouch(0, 0);
//	inputManager->createWlTouch(0, 1);

	return inputManager;
}

Input::InputManagerPtr getInputManager()
{
	auto inputManager = new Input::InputManager();

	return Input::InputManagerPtr(inputManager);
}

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{

			DisplayItf::DisplayPtr display;
			shared_ptr<Wayland::Display> wlDisplay;
			InputItf::InputManagerPtr inputManager;


			if (gDisplayMode == DisplayMode::DRM)
			{
				auto drmDisplay = getDrmDisplay();

				inputManager = getInputManager();

				display = drmDisplay;
			}
			else
			{
				wlDisplay = getWaylandDisplay();

				inputManager = getWlInputManager(wlDisplay);

				display = wlDisplay;
			}

			DisplayBackend displayBackend(display, XENDISPL_DRIVER_NAME, 0, 0);
			InputBackend inputBackend(inputManager, XENKBD_DRIVER_NAME, 0, 0);

			displayBackend.start();
			inputBackend.start();

			waitSignals();
		}
		else
		{
			cout << "Usage: " << argv[0] << " [-m <mode>] [-v <level>]" << endl;
			cout << "\t-v -- verbose level "
				 << "(disable, error, warning, info, debug)" << endl;
			cout << "\t-m -- mode "
				 << "(drm, wayland)" << endl;
		}
	}
	catch(const std::exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}

	return 0;
}
