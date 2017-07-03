#include <iostream> // cout
#include <thread> // this_thread::sleep_for, this_thread::yield
#include <chrono> // milliseconds
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <conio.h> // _kbhit, _getch_nolock
#include "XOutput.hpp"

#include "Common.hpp"
#include "Controller.hpp"
#include "Cerberus.hpp"
#include "Version.hpp"

namespace {
	bool hasBroke{ false };

	void unsetBreakHandler();
	BOOL __stdcall breakHandler(DWORD type) {
		switch (type) {
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_BREAK_EVENT:
			hasBroke = true;
			unsetBreakHandler();
			return TRUE;
		}
		return FALSE;
	}

	void setBreakHandler() {
		SetConsoleCtrlHandler(breakHandler, TRUE);
	}
	void unsetBreakHandler() {
		SetConsoleCtrlHandler(breakHandler, FALSE);
	}

	constexpr bool SUCCESS(DWORD e) {
		return e == XOutput::XOUTPUT_SUCCESS;
	}

	void pause() {
		while (_kbhit() != 0) _getch(); // Eat any buffered input
		std::cout << "Press any key to continue..." << std::endl; // Intentional use of endl to flush output buffer
		while (_kbhit() == 0) { // Wait for any input
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

// argc and argv are unused
int main(int, char*[]) {
	using std::cout;
	using std::this_thread::yield;
	using namespace Procon;

	// Pause before exiting
	auto pause = make_scoped(::pause);

	cout << ProgramName << ' ' << ProgramVersion << ' ' << Platform << ' ' << BuildType << "\n\n";

	try {
		XOutput::XOutputInitialize();
	}
	catch (XOutput::XOutputError &e) {
		cout << e.what() << '\n';
		return -1;
	}

	DWORD unused;
	if (!SUCCESS(XOutput::XOutputGetRealUserIndex(0, &unused))) {
		cout << "Unable to connect to ScpVBus.\n";
		return -1;
	}

#ifndef NO_CERBERUS
	Cerberus cerb;
	try {
		cerb.init();
		cout << "Initialized HidCerberus.\n";
	}
	catch (CerberusError &e) {
		cout << "Unable to initialize Cerberus.\n";
		cout << "Error: " << e.what() << '\n';
		cout << "Continuing, may not find the controller if it's hidden by HidGuardian.\n";
	}
#endif
	
	std::vector<Controller> cs;
	uchar port{ 0 };
	{
		constexpr auto id = Procon_ID; // Procon only for now
		constexpr auto vendorId = NintendoID;
		hid_device_info * const devs = hid_enumerate(vendorId, id); // Don't trust hidapi, returns non-matching devices sometimes (*const to prevent compiler from optimizing away)
		hid_device_info *iter = devs;
		do {
			if (iter != nullptr) {
				if (iter->product_id == id) { // Check the id!
					try {
						cs.emplace_back(Controller(port++));
						cs.back().openDevice(iter);
					}
					catch (ControllerException &e) {
						cout << "Exception connecting to controller: " << e.what() << '\n';
						return -1;
					}
				}
				iter = iter->next;
			}
		} while (iter != nullptr && port < 4);
		hid_free_enumeration(devs);
	}
	if (cs.size() == 0) {
		cout << "Unable to find controller.\n";
		return -1;
	}

	cout << "\nConnected to " << static_cast<int>(port) << " controller(s). Beginning xInput emulation.\n\n";
	
	cout << "Doing calibration, stick min/maxes will be updated automatically.\n";
	cout << "Move the sticks some, then let them reset to neutral.\n";
	cout << "Press the Share button to set stick center. This only works once per controller currently!\n";
	cout << "XInput may be laggier before stick center for all controllers is set!\n\n";
	
	cout << "Press CTRL+C to exit.\n\n";
	::setBreakHandler();

	std::array<bool, 4> hasCentered;
	hasCentered.fill(false);

	try {
		// Testing to set centers, additional comparisons = slower so make it a separate loop
		size_t countCentered{ 0 };
		while (!::hasBroke && countCentered < port) {
			for (size_t i = 0; i < port; ++i) {
				cs[i].pollInput();
				if (!hasCentered[i]) {
					const Procon::ExpandedPadState &state = cs[i].getState();
					if (state.sharePressed) {
						cs[i].setCalibrationCenter(state.leftStick, state.rightStick);
						hasCentered[i] = true;
						++countCentered;
						cout << "Set stick centers for controller LED " << i + 1 << '\n';
					}
				}
			}
			yield();
		}
		cout << "\nAll controller stick centers set, entering fast input loop. Enjoy your games!\n";
		// Centers set, main input loop
		while(!::hasBroke){
			for (size_t i = 0; i < port; ++i) {
				cs[i].pollInput();
			}
			yield(); // sleep_for causes big lag and not yielding eats way more processor
		}
	}
	catch (ControllerException &e) {
		cout << "ControllerException: " << e.what() << '\n';
		return -1;
	}

	return 0;
}