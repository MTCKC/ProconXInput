#include <iostream> // cout
#include <thread> // this_thread::sleep_for
#include <chrono> // milliseconds

#include <Windows.h>
#include <conio.h> // _kbhit, _getch_nolock
#include <ViGEmUM.h>

#include "Common.hpp"
#include "Controller.hpp"
#include "Cerberus.hpp"

#ifdef VIGEM_SUCCESS
// Write it as a function, not a #define
#undef VIGEM_SUCCESS
#endif

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

	inline bool VIGEM_SUCCESS(VIGEM_ERROR e) {
		return e == VIGEM_ERROR_NONE;
	}

	void pause() {
		while (_kbhit() != 0) _getch_nolock(); // Eat any buffered input
		std::cout << "Press any key to continue..." << std::endl; // Intentional use of endl to flush output buffer
		while (_kbhit() == 0) { // Wait for any input
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

// argc and argv are unused
int main(int, char*[]) {
	using std::cout;
	using namespace Procon;
	using namespace std::this_thread; // sleep_for
	using namespace std::chrono; // milliseconds

	// Pause before exiting
	auto pause = make_scoped(::pause);

	if (!VIGEM_SUCCESS(vigem_init())) {
		cout << "Unable to initialize ViGEm.\n";
		return -1;
	}
	
	auto shutdownViGEm = make_scoped(vigem_shutdown);
	
	// Initialize HidCerberus. Comment out until EndCerberus to remove HidCerberus support.
	Cerberus cerb;
	try {
		cerb.init();
	}
	catch (CerberusError &e) {
		cout << "Unable to initialize Cerberus.\n";
		cout << "Error: " << e.what() << '\n';
		return -1;
	}
	// EndCerberus
	
	Controller c;
	{
		constexpr auto id = Procon_ID; // Procon only for now
		constexpr auto vendorId = NintendoID;
		hid_device_info * const devs = hid_enumerate(vendorId, id); // Don't trust hidapi, returns non-matching devices sometimes (*const to prevent compiler from optimizing away)
		hid_device_info *iter = devs;
		do {
			if (iter != nullptr) {
				if (iter->product_id == id) { // Check the id!
					try {
						c.openDevice(iter);
					}
					catch (ControllerException &e) {
						cout << "Exception connecting to controller: " << e.what() << '\n';
						return -1;
					}
				}
				iter = iter->next;
			}
		} while (!c.connected() && iter != nullptr);
		hid_free_enumeration(devs);
	}
	if (!c.connected()) {
		cout << "Unable to find controller.\n";
		return -1;
	}

	cout << "Connected to controller. Beginning xInput emulation.\n";
	cout << "Press CTRL+C to exit.\n";
	::setBreakHandler();
	while (!::hasBroke) {
		c.pollInput();
		sleep_for(milliseconds(1));
	}

	return 0;
}