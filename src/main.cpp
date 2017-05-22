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

#include "Config.hpp"
#include "drm/Display.hpp"
#include "DisplayBackend.hpp"
#include "input/InputManager.hpp"
#include "InputBackend.hpp"

using std::atomic_bool;
using std::chrono::milliseconds;
using std::cout;
using std::dynamic_pointer_cast;
using std::endl;
using std::string;
using std::this_thread::sleep_for;
using std::toupper;
using std::transform;

using XenBackend::Log;

const uint32_t cWlBackgroundWidth = 1920;
const uint32_t cWlBackgroundHeight = 1080;

string gCfgFileName;

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

	while((opt = getopt(argc, argv, "c:v:fh?")) != -1)
	{
		switch(opt)
		{
		case 'v':

			if (!Log::setLogLevel(string(optarg)))
			{
				return false;
			}

			break;

		case 'c':

			gCfgFileName = optarg;

			break;

		case 'f':

			Log::setShowFileAndLine(true);

			break;

		default:
			return false;
		}
	}

	return true;
}

DisplayItf::DisplayPtr getDisplay(ConfigPtr config)
{
	// DRM

	if (config->displayMode() == Config::DisplayMode::DRM)
	{
		return Drm::DisplayPtr(new Drm::Display("/dev/dri/card0"));
	}

	// Wayland

	Wayland::DisplayPtr wlDisplay(new Wayland::Display());

	string name;
	uint32_t screen, x, y, w, h, z;

	if (config->wlBackground(w, h))
	{
		wlDisplay->createBackgroundSurface(w, h);
	}

	for (int i = 0; i < config->wlConnectorsCount(); i++)
	{
		config->wlConnector(i, name, screen, x, y, w, h, z);

		wlDisplay->createConnector(name, screen, x, y, w, h, z);
	}

	return wlDisplay;
}

InputItf::InputManagerPtr getInputManager(DisplayItf::DisplayPtr display,
										  ConfigPtr config)
{
	Input::InputManagerPtr inputManager;

	if (config->displayMode() == Config::DisplayMode::WAYLAND)
	{
		inputManager.reset(new Input::InputManager(
				dynamic_pointer_cast<Wayland::Display>(display)));
	}
	else
	{
		inputManager.reset(new Input::InputManager());
	}

	int id;
	bool wayland;
	string name;

	for(int i = 0; i < config->inputKeyboardsCount(); i++)
	{
		config->inputKeyboard(i, id, wayland, name);

		if (wayland)
		{
			if (config->displayMode() != Config::DisplayMode::WAYLAND)
			{
				throw InputItf::Exception(
						"Can't create wayland keyboard. Wayland is disabled.");
			}

			inputManager->createWlKeyboard(id, name);
		}
		else
		{
			inputManager->createInputKeyboard(id, name);
		}
	}

	for(int i = 0; i < config->inputPointersCount(); i++)
	{
		config->inputPointer(i, id, wayland, name);

		if (wayland)
		{
			if (config->displayMode() != Config::DisplayMode::WAYLAND)
			{
				throw InputItf::Exception(
						"Can't create wayland pointer. Wayland is disabled.");
			}

			inputManager->createWlPointer(id, name);
		}
		else
		{
			inputManager->createInputPointer(id, name);
		}
	}

	for(int i = 0; i < config->inputTouchesCount(); i++)
	{
		config->inputTouch(i, id, wayland, name);

		if (wayland)
		{
			if (config->displayMode() != Config::DisplayMode::WAYLAND)
			{
				throw InputItf::Exception(
						"Can't create wayland touch. Wayland is disabled.");
			}

			inputManager->createWlTouch(id, name);
		}
		else
		{
			inputManager->createInputTouch(id, name);
		}
	}

	return inputManager;
}

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{
			ConfigPtr config(new Config(gCfgFileName));

			DisplayItf::DisplayPtr display = getDisplay(config);
			InputItf::InputManagerPtr inputManager = getInputManager(display,
																	 config);

			DisplayBackend displayBackend(config, display,
										  XENDISPL_DRIVER_NAME, 0);
			InputBackend inputBackend(config, inputManager,
									  XENKBD_DRIVER_NAME, 0);

			displayBackend.start();
			inputBackend.start();

			waitSignals();

			displayBackend.stop();
			inputBackend.stop();
		}
		else
		{
			cout << "Usage: " << argv[0] << " [-m <mode>] [-v <level>]" << endl;
			cout << "\t-v -- verbose level "
				 << "(disable, error, warning, info, debug)" << endl;
			cout << "\t-c -- config file" << endl;
		}
	}
	catch(const std::exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}

	return 0;
}
