#include "Controller.hpp"

#include <array>
#include <iostream>
#include <vector>
#include <limits>

#include <ViGEmUM.h>

#include "hidapi.h"


namespace Procon {
	using std::array;

	void HIDCloser::operator()(hid_device *ptr) {
		if (ptr != nullptr)
			hid_close(ptr);
	}

	Controller::Controller() :vController(), device(nullptr) {
		VIGEM_TARGET_INIT(&vController);
	}
	Controller::Controller(Controller &&) = default;
	Controller& Controller::operator=(Controller &&) = default;
	Controller::~Controller() {
		if (vController.State == VIGEM_TARGET_CONNECTED) {
			vigem_target_unplug(&vController);
		}
		if (device) {
			static const array<uchar, 2> disconnect{ 0x80, 0x05 };
			exchange(disconnect);
		}
	}
};
namespace {
	using std::array;
	using Procon::uchar;

	// openDevice
	const array<uchar, 2> getMAC{ 0x80, 0x01 };
	const array<uchar, 2> handshake{ 0x80, 0x02 };
	const array<uchar, 2> switchBaudrate{ 0x80, 0x03 };
	const array<uchar, 2> HIDOnlyMode{ 0x80, 0x04 };
	const array<uchar, 1> enable{ 0x01 };
	constexpr uchar rumbleCommand{ 0x48 };
	constexpr uchar imuDataCommand{ 0x40 };

	constexpr uchar ledCommand{ 0x30 };
	const array<uchar, 1> led{ 0x1 };

	// pollInput
	constexpr uchar getInput{ 0x1f };
	const array<uchar, 0> empty{};

	struct InputPacket {
		uint8_t header[8];
		uint8_t unknown[5];
		uint8_t rightButtons;
		uint8_t middleButtons;
		uint8_t leftButtons;
		uint8_t sticks[6];
	};

	short expandUChar(uchar c) {
		int big = c;
		big *= std::numeric_limits<short>::max() + 1;
		big /= 128;
		big -= std::numeric_limits<short>::max() + 1;
		return static_cast<short>(big);
	}

	VOID CALLBACK testNotification(VIGEM_TARGET t, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LEDNumber) {
		std::cout << "LED:" << static_cast<int>(LEDNumber) << " Serial: " << t.SerialNo << " ProductID: " << t.ProductId
			<< " VendorID: " << t.VendorId << '\n';
	}

};
namespace Procon {
	void Controller::openDevice(hid_device_info *dev) {
		if (dev == nullptr)
			throw ControllerException("Unable to open controller device: dev was nullptr.");
		if (dev->product_id != Procon_ID)
			throw ControllerException("Unable to open controller device: product id was not a Switch Pro Controller.");
		device.reset(hid_open_path(dev->path));
		if (!device)
			throw ControllerException("Unable to open controller device: device path could not be opened.");
		//vController.ProductId = dev->product_id;
		//vController.VendorId = dev->vendor_id;
		if (!exchange(handshake)) {
			throw ControllerException("Handshake failed.");
		}
		exchange(switchBaudrate);
		exchange(handshake);
		exchange(HIDOnlyMode);
		sendSubcommand(0x1, rumbleCommand, enable);
		sendSubcommand(0x1, imuDataCommand, enable);
		 
		sendSubcommand(0x1, ledCommand, led);

		if (vigem_target_plugin(Xbox360Wired, &vController) != VIGEM_ERROR_NONE) {
			device.reset(nullptr);
			throw ControllerException("Unable to plugin ViGEm controller.");
		}

		vigem_register_xusb_notification(testNotification, vController);


	}

};
namespace {
	using std::vector;
	using std::tuple;
	using std::array;

	using namespace Procon;

	const array<Button, 8>& getButtonMap(ButtonSource s) {
		switch (s) {
		case ButtonSource::Left:
			return JoyconLBitmap;
		case ButtonSource::Middle:
			return JoyconMidBitmap;
		case ButtonSource::Right:
			return JoyconRBitmap;
		default:
			throw std::logic_error("Unknown ButtonSource passed to getButtonMap");
		}
	}

	void pullButtonsFromByte(uchar c, ButtonSource src, vector<tuple<Button, bool>> &out) {
		const array<Button, 8>& map = getButtonMap(src);
		for (uchar i{ 0 }; i < 8; ++i) {
			if (map[i] == Button::None) continue;
			out.push_back(std::make_tuple(map[i], (c & (1 << i)) != 0));
		}
	}

	constexpr unsigned short buttonToReportBits(Button b){
		switch (b) {
		case Button::DPadUp:
			return 0x0001;
		case Button::DPadDown:
			return 0x0002;
		case Button::DPadLeft:
			return 0x0004;
		case Button::DPadRight:
			return 0x0008;
		case Button::Plus:
			return 0x0010;
		case Button::Minus:
			return 0x0020;
		case Button::LStick:
			return 0x0040;
		case Button::RStick:
			return 0x0080;
		case Button::L:
			return 0x0100;
		case Button::R:
			return 0x0200;
		case Button::Home:
			return 0x0400; // Undocumented
		case Button::A:
			return 0x1000;
		case Button::B:
			return 0x2000;
		case Button::X:
			return 0x4000;
		case Button::Y:
			return 0x8000;
		default:
			return 0x0000;
		}
	}

	void mapButtonToReport(Button b, XUSB_REPORT &report) {
		switch (b) {
		case Button::LZ:
			report.bLeftTrigger = std::numeric_limits<BYTE>::max();
			return;
		case Button::RZ:
			report.bRightTrigger = std::numeric_limits<BYTE>::max();
			return;
		default:
			report.wButtons |= buttonToReportBits(b);
			return;
		}
	}

	void mapInputToReport(const InputPacket &p, XUSB_REPORT &report) {
		uchar LeftX = ((p.sticks[1] & 0x0F) << 4) | ((p.sticks[0] & 0xF0) >> 4);
		uchar LeftY = p.sticks[2];
		uchar RightX = ((p.sticks[4] & 0x0F) << 4) | ((p.sticks[3] & 0xF0) >> 4);
		uchar RightY = p.sticks[5];

		report.sThumbLX = expandUChar(LeftX);
		report.sThumbLY = expandUChar(LeftY);
		report.sThumbRX = expandUChar(RightX);
		report.sThumbRY = expandUChar(RightY);

		vector<tuple<Button, bool>> buttons;
		pullButtonsFromByte(p.leftButtons, ButtonSource::Left, buttons);
		pullButtonsFromByte(p.rightButtons, ButtonSource::Right, buttons);
		pullButtonsFromByte(p.middleButtons, ButtonSource::Middle, buttons);

		for (auto pair : buttons) {
#ifdef _DEBUG
			//std::cout << "Button " << buttonToString(std::get<0>(pair)) << " is " << (std::get<1>(pair) ? " pressed\n" : " up\n");
#endif
			if (std::get<1>(pair)) {
				mapButtonToReport(std::get<0>(pair), report);
			}
		}
	}
};

namespace Procon {

	void Controller::pollInput() {
		if (!device)
			return;
		XUSB_REPORT report;
		memset(&report, 0, sizeof(XUSB_REPORT));
		auto dat = sendCommand(getInput, empty);
		if (!dat) {
			throw ControllerException("Error sending getInput command.");
		}
		if (dat.value()[0] != 0x30) {
			InputPacket p;
			memcpy(&p, dat.value().data(), sizeof(InputPacket));

			mapInputToReport(p, report);

			vigem_xusb_submit_report(vController, report);
		}
	}

	bool Controller::connected() const {
		return vController.State == VIGEM_TARGET_CONNECTED;
	}

	ControllerException::ControllerException(const std::string& what) : runtime_error(what) {}
	ControllerException::ControllerException(const char* what) : runtime_error(what) {}
};
namespace {
	using std::string;

	const string buttonA{ "A" };
	const string buttonB{ "B" };
	const string buttonX{ "X" };
	const string buttonY{ "Y" };
	const string buttonLStick{ "Left Stick" };
	const string buttonRStick{ "Right Stick" };
	const string buttonL{ "L" };
	const string buttonR{ "R" };
	const string buttonLZ{ "LZ" };
	const string buttonRZ{ "RZ" };
	const string buttonHome{ "Home" };
	const string buttonShare{ "Share" };
	const string buttonPlus{ "Plus" };
	const string buttonMinus{ "Minus" };
	const string buttonDPUp{ "DPad Up" };
	const string buttonDPLeft{ "DPad Left" };
	const string buttonDPRight{ "DPad Right" };
	const string buttonDPDown{ "DPad Down" };
	const string buttonNone{ "None" };
	const string buttonUnknown{ "Unknown" };
};
	const std::string& Procon::buttonToString(Button b) {
		switch (b) {
		case Button::A:
			return buttonA;
		case Button::B:
			return buttonB;
		case Button::X:
			return buttonX;
		case Button::Y:
			return buttonY;
		case Button::LStick:
			return buttonLStick;
		case Button::RStick:
			return buttonRStick;
		case Button::L:
			return buttonL;
		case Button::LZ:
			return buttonLZ;
		case Button::R:
			return buttonR;
		case Button::RZ:
			return buttonRZ;
		case Button::Home:
			return buttonHome;
		case Button::Share:
			return buttonShare;
		case Button::Plus:
			return buttonPlus;
		case Button::Minus:
			return buttonMinus;
		case Button::DPadDown:
			return buttonDPDown;
		case Button::DPadUp:
			return buttonDPUp;
		case Button::DPadLeft:
			return buttonDPLeft;
		case Button::DPadRight:
			return buttonDPRight;
		case Button::None:
			return buttonNone;
		default:
			return buttonUnknown;
		}
	}

