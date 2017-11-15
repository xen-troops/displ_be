#include <csignal>
#include <execinfo.h>
#include <getopt.h>
#include <unistd.h>

#include <xen/be/Log.hpp>

#include "wayland/Display.hpp"
#include "input/WlInput.hpp"

using XenBackend::Log;
/*******************************************************************************
 *
 ******************************************************************************/

void onMoveRelative(int32_t x, int32_t y, int32_t relZ)
{
	LOG("Main", DEBUG) << "Move REL x = " << x << " y = " << y;
}

void onMoveAbsolute(int32_t x, int32_t y, int32_t relZ)
{
	LOG("Main", DEBUG) << "Move ABS x = " << x << " y = " << y;
}

void onButtons(uint32_t button, uint32_t state)
{
	LOG("Main", DEBUG) << "Buttons btn = " << button << " state = " << state;
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
	Log::setLogLevel("Debug");

	LOG("Main", DEBUG) << "Start";

	try
	{
		registerSignals();

		auto display = Wayland::DisplayPtr(new Wayland::Display());

		display->start();

		auto con1 = display->createConnector("1");
		auto con2 = display->createConnector("2");

		auto db1 = display->createDisplayBuffer(300, 200, 32);
		auto fb1 = display->createFrameBuffer(db1, 300, 200, 1);

		auto db2 = display->createDisplayBuffer(300, 200, 32);
		auto fb2 = display->createFrameBuffer(db2, 300, 200, 1);

		memset(db1->getBuffer(), db1->getSize(), 0xFF);
		memset(db2->getBuffer(), db2->getSize(), 0xAA);

		con1->init(300, 200, fb1);
		con2->init(300, 200, fb2);

		WlPointer pointer(display, "1");

		pointer.setCallbacks({onMoveRelative, onMoveAbsolute, onButtons});

//		waitSignals();

		display->stop();
	}
	catch(const std::exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}

	LOG("Main", DEBUG) << "Stop";

	return 0;
}
