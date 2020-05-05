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
#endif

#ifdef WITH_MOCKBELIB
#include "MockBackend.hpp"
#endif

#include "Version.hpp"

using std::cout;
using std::dynamic_pointer_cast;
using std::endl;
using std::ofstream;
using std::string;
using std::this_thread::sleep_for;
using std::toupper;
using std::transform;
using std::vector;

using XenBackend::Log;
using XenBackend::Utils;

enum class DisplayMode
{
	WAYLAND,
	DRM
};

DisplayMode gDisplayMode = DisplayMode::WAYLAND;
string gDrmDevice = "/dev/dri/card0";
string gLogFileName;
bool gDisableZCopy = false;

int gRetStatus = EXIT_SUCCESS;

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

	raise(sig);
}

void registerSignals()
{
	struct sigaction act {};

	act.sa_handler = segmentationHandler;
	act.sa_flags = SA_RESETHAND;

	sigaction(SIGSEGV, &act, nullptr);
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

	if (signal == SIGTERM)
	{
		gRetStatus = EXIT_FAILURE;
	}
}

bool commandLineOptions(int argc, char *argv[])
{
	int opt = -1;
#ifdef WITH_ZCOPY
	static const char* optString = "m:d:v:l:f:hz?";
#else
	static const char* optString = "m:d:v:l:f:h?";
#endif

	while((opt = getopt(argc, argv, optString)) != -1)
	{
		switch(opt)
		{
		case 'm':
		{
			string mode = optarg;

			transform(mode.begin(), mode.end(), mode.begin(),
					  (int (*)(int))toupper);

			if (mode == "DRM")
			{
				gDisplayMode = DisplayMode::DRM;
			}
			else if (mode == "WAYLAND")
			{
				gDisplayMode = DisplayMode::WAYLAND;
			}
			else
			{
				return false;
			}

			break;
		}

		case 'd':

			gDrmDevice = optarg;

			break;

		case 'v':

			if (!Log::setLogMask(string(optarg)))
			{
				return false;
			}

			break;

		case 'l':

			gLogFileName = optarg;

			break;

		case 'f':

			Log::setShowFileAndLine(true);

			break;

#ifdef WITH_ZCOPY
		case 'z':

			gDisableZCopy = true;

			break;
#endif

		default:

			return false;
		}
	}

	return true;
}

#ifdef WITH_DISPLAY
DisplayItf::DisplayPtr getDisplay(DisplayMode mode)
{
	if (mode == DisplayMode::DRM)
	{
#ifdef WITH_DRM
		// DRM
		return Drm::DisplayPtr(new Drm::Display(gDrmDevice, gDisableZCopy));
#else
		throw XenBackend::Exception("DRM mode is not supported", EINVAL);
#endif
	}
	else
	{
#ifdef WITH_WAYLAND
		// Wayland
		return Wayland::DisplayPtr(new Wayland::Display(gDisableZCopy));
#else
		throw XenBackend::Exception("WAYLAND mode is not supported", EINVAL);
#endif
	}
}
#endif

int main(int argc, char *argv[])
{
	try
	{
		registerSignals();

		if (commandLineOptions(argc, argv))
		{
			LOG("Main", INFO) << "backend version:  " << VERSION;
			LOG("Main", INFO) << "libxenbe version: " << Utils::getVersion();

			ofstream logFile;

			if (!gLogFileName.empty())
			{
				logFile.open(gLogFileName);
				Log::setStreamBuffer(logFile.rdbuf());
			}

#ifdef WITH_MOCKBELIB
			MockBackend mockBackend(0, 1);
#endif

#ifdef WITH_DISPLAY
			auto display = getDisplay(gDisplayMode);

			DisplayBackend displayBackend(display, XENDISPL_DRIVER_NAME);

			displayBackend.start();
#endif

#ifdef WITH_INPUT
#ifdef WITH_WAYLAND
			InputBackend inputBackend("vinput",
					dynamic_pointer_cast<Wayland::Display>(display));
#else
			InputBackend inputBackend("vinput");
#endif
			inputBackend.start();
#endif
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
				 << " [-l <file>] [-v <level>]"
				 << endl;
			cout << "\t-m -- mode: DRM or WAYLAND" << endl;
#ifdef WITH_ZCOPY
			cout << "\t-z -- disable zero-copy" << endl;
#endif
			cout << "\t-d -- DRM device" << endl;
			cout << "\t-l -- log file" << endl;
			cout << "\t-v -- verbose level in format: "
				 << "<module>:<level>;<module:<level>" << endl;
			cout << "\t      use * for mask selection:"
				 << " *:Debug,Mod*:Info" << endl;

			gRetStatus = EXIT_FAILURE;
		}
	}
	catch(const std::exception& e)
	{
		Log::setStreamBuffer(cout.rdbuf());

		LOG("Main", ERROR) << e.what();

		gRetStatus = EXIT_FAILURE;
	}

	return gRetStatus;
}
