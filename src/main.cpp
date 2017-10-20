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
#include <atomic>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>

#include <csignal>
#include <execinfo.h>
#include <getopt.h>
#include <unistd.h>

#include <xen/be/Log.hpp>

#include "Config.hpp"

#ifdef WITH_DISPLAY
#include "DisplayBackend.hpp"
#ifdef WITH_DRM
#include "drm/Display.hpp"
#endif //WITH_DRM
#ifdef WITH_WAYLAND
#include "wayland/Display.hpp"
#endif //WITH_WAYLAND
#endif //WITH_DISPLAY

#ifdef WITH_INPUT
#include "InputBackend.hpp"
#include "input/InputManager.hpp"
#endif

using std::cout;
using std::dynamic_pointer_cast;
using std::endl;
using std::ofstream;
using std::string;
using std::this_thread::sleep_for;
using std::toupper;
using std::transform;

using XenBackend::Log;

string gCfgFileName;
string gLogFileName;

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

	while((opt = getopt(argc, argv, "c:v:l:fh?")) != -1)
	{
		switch(opt)
		{
		case 'v':

			if (!Log::setLogMask(string(optarg)))
			{
				return false;
			}

			break;

		case 'c':

			gCfgFileName = optarg;

			break;

		case 'l':

			gLogFileName = optarg;

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

#ifdef WITH_DISPLAY
DisplayItf::DisplayPtr getDisplay(ConfigPtr config)
{
#ifdef WITH_DRM
	// DRM
	if (config->displayMode() == Config::DisplayMode::DRM)
	{
		return Drm::DisplayPtr(new Drm::Display("/dev/dri/card0"));
	}
#endif

#ifdef WITH_WAYLAND
	// Wayland
	Wayland::DisplayPtr wlDisplay(new Wayland::Display());

	for (int i = 0; i < config->displayDomsCount(); i++)
	{
		string domName;
		uint16_t devId;
		int connectorsCount;

		config->displayDomParams(i, domName, devId, connectorsCount);

		for (int j = 0; j < connectorsCount; j++)
		{
			wlDisplay->createConnector(
					config->displayDomConnectorName(domName, devId, j));
		}
	}

	return wlDisplay;
#else
	throw DisplayItf::Exception("Wayland is not supported");
#endif
}
#endif

#ifdef WITH_INPUT
InputItf::InputManagerPtr getInputManager(
#ifdef WITH_WAYLAND
										  Wayland::DisplayPtr display,
#endif
										  ConfigPtr config
		)
{
	Input::InputManagerPtr inputManager(new Input::InputManager());

	for (int i = 0; i < config->inputDomsCount(); i++)
	{
		string connector;
		string device;

		config->inputKeyboard(i, device, connector);

		if (!connector.empty())
		{
#ifdef WITH_WAYLAND
			inputManager->createWlKeyboard(i, connector);
#else
			throw InputItf::Exception(
					"Can't create wayland keyboard. Wayland is not supported.");
#endif
		}

		if (!device.empty())
		{
			inputManager->createInputKeyboard(i, device);
		}

		config->inputPointer(i, connector, device);

		if (!connector.empty())
		{
#ifdef WITH_WAYLAND
			inputManager->createWlPointer(i, connector);
#else
			throw InputItf::Exception(
					"Can't create wayland keyboard. Wayland is not supported.");
#endif
		}

		if (!device.empty())
		{
			inputManager->createInputPointer(i, device);
		}

		config->inputTouch(i, connector, device);

		if (!connector.empty())
		{
#ifdef WITH_WAYLAND
			inputManager->createWlTouch(i, connector);
#else
			throw InputItf::Exception(
					"Can't create wayland keyboard. Wayland is not supported.");
#endif
		}

		if (!device.empty())
		{
			inputManager->createInputTouch(i, device);
		}
	}

	return inputManager;
}

#endif //WITH_INPUT

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{
			ofstream logFile;

			if (!gLogFileName.empty())
			{
				logFile.open(gLogFileName);
				Log::setStreamBuffer(logFile.rdbuf());
			}

			ConfigPtr config(new Config(gCfgFileName));

#ifdef WITH_DISPLAY
			DisplayItf::DisplayPtr display = getDisplay(config);
			DisplayBackend displayBackend(config, display,
										  XENDISPL_DRIVER_NAME);
			displayBackend.start();
#endif

#ifdef WITH_INPUT
			InputItf::InputManagerPtr inputManager;

			inputManager =
#ifdef WITH_WAYLAND
					getInputManager(
							dynamic_pointer_cast<Wayland::Display>(display),
							config);
#else
			getInputManager(config);
#endif

			InputBackend inputBackend(config, inputManager,
									  XENKBD_DRIVER_NAME);

			inputBackend.start();
#endif //WITH_INPUT

			waitSignals();

#ifdef WITH_DISPLAY
			displayBackend.stop();
#endif

#ifdef WITH_INPUT
			inputBackend.stop();
#endif
			logFile.close();
		}
		else
		{
			cout << "Usage: " << argv[0]
				 << " [-c <file>] [-l <file>] [-v <level>]"
				 << endl;
			cout << "\t-c -- config file" << endl;
			cout << "\t-l -- log file" << endl;
			cout << "\t-v -- verbose level in format: "
				 << "<module>:<level>;<module:<level>" << endl;
			cout << "\t      use * for mask selection:"
				 << " *:Debug,Mod*:Info" << endl;
		}
	}
	catch(const std::exception& e)
	{
		Log::setStreamBuffer(cout.rdbuf());

		LOG("Main", ERROR) << e.what();
	}

	return 0;
}
