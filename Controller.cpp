#include "Controller.hpp"

#include <array>
#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif
#include <string>
#include <limits>
#include <vector>

#include "hidapi.h"
#include "XOutput.hpp"
#include "Config.hpp"

using namespace XOutput;

namespace Procon {
	using std::array;

	void SetDefaultCalibration(CalibrationData &dat) {
		constexpr uchar ucmin = std::numeric_limits<uchar>::min();
		constexpr uchar ucmax = std::numeric_limits<uchar>::max();
		dat.leftCenter.x = ucmax / 2;
		dat.leftCenter.y = ucmax / 2;
		dat.rightCenter = dat.leftCenter;
		dat.left.x.min = dat.leftCenter.x;
		dat.left.x.max = dat.leftCenter.x;
		dat.left.y = dat.left.x;
		dat.right = dat.left;
	}

	void HIDCloser::operator()(hid_device *ptr) {
		if (ptr != nullptr)
			hid_close(ptr);
	}
	void zeroPadState(ExpandedPadState &state) {
		state.xinState = { 0 };
		state.leftStick = { 0 };
		state.rightStick = { 0 };
		state.sharePressed = false;
	}
	Controller::Controller(uchar port) :device(nullptr), port(port) {
		SetDefaultCalibration(calib);
	}
	Controller::Controller(Controller &&) = default;
	Controller& Controller::operator=(Controller &&) = default;
	Controller::~Controller() {
		if (_connected) {
			XOutputUnPlug(port);
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
	using Procon::StickPoint;
	using Procon::StickRange;
	using Procon::CalibrationData;

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

	constexpr double lerp(double min, double max, double t) {
		return (1.0 - t) * min + t * max;
	}

	// stick is current stick location, range is min/max of stick, center is center point of stick
	void calibrateToRange(const StickPoint &stick, const StickRange &range, const StickPoint &center, short &outx, short &outy) {
		constexpr short smax = std::numeric_limits<short>::max();
		constexpr short smin = std::numeric_limits<short>::min();
		
		outx = static_cast<short>(
			smax * std::clamp(
				(static_cast<double>(stick.x) - center.x) / static_cast<double>(range.x.max - range.x.min) * 2.0,
				-1.0,
				1.0
			)
			
		);
		outy = static_cast<short>(
			smax *std::clamp(
				(static_cast<double>(stick.y) - center.y) / static_cast<double>(range.y.max - range.y.min) * 2.0,
				-1.0,
				1.0
			)
		);

	}

	short expandUChar(uchar c) {
		constexpr uchar ucmax = std::numeric_limits<uchar>::max();
		constexpr short smax = std::numeric_limits<short>::max();
		constexpr short smin = std::numeric_limits<short>::min();

		double d = std::clamp(static_cast<double>(c) / ucmax, 0.0, 1.0);
		return static_cast<short>( lerp(smin, smax, d) );
	}

};
namespace Procon {

	void Controller::openDevice(hid_device_info *dev) {
		using namespace std::this_thread;
		using namespace std::chrono;

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

		if (XOutputPlugIn(port) != ERROR_SUCCESS) {
			device.reset(nullptr);
			throw ControllerException("Unable to plugin XOutput controller.");
		}
		_connected = true;
		sleep_for(milliseconds(100));
		//updateStatus();
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

		// NOTICE: A and B are swapped, and X and Y are swapped.
		case Button::A:
			return 0x2000;
		case Button::B:
			return 0x1000;
		case Button::X:
			return 0x8000;
		case Button::Y:
			return 0x4000;
		default:
			return 0x0000;
		}
	}
	constexpr unsigned short buttonToReportBitsOld(Button b) {
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

	const std::string buttonConfigName{ "bMatchButtonLabels" };

	void mapButtonToState(Button b, ExpandedPadState &state) {
		switch (b) {
		case Button::LZ:
			state.xinState.bLeftTrigger = std::numeric_limits<BYTE>::max();
			return;
		case Button::RZ:
			state.xinState.bRightTrigger = std::numeric_limits<BYTE>::max();
			return;
		case Button::Share:
			state.sharePressed = true;
			return;
		default:
			if (Config::get<bool>(buttonConfigName).value_or(false)) {
				state.xinState.wButtons |= buttonToReportBitsOld(b);
			}
			else {
				state.xinState.wButtons |= buttonToReportBits(b);
			}
			return;
		}
	}

	void updateCalibrationRangeStick(const StickPoint &input, StickRange &cal) {
		using std::max;
		using std::min;

		cal.x.max = max(cal.x.max, input.x);
		cal.x.min = min(cal.x.min, input.x);
		cal.y.max = max(cal.y.max, input.y);
		cal.y.min = min(cal.y.min, input.y);
		
	}

	void updateCalibrationRange(const ExpandedPadState &state, CalibrationData &cal) {
		updateCalibrationRangeStick(state.leftStick, cal.left);
		updateCalibrationRangeStick(state.rightStick, cal.right);
	}

	void mapInputToState(const InputPacket &p, CalibrationData &cal, ExpandedPadState &state) {
		state.leftStick.x = ((p.sticks[1] & 0x0F) << 4) | ((p.sticks[0] & 0xF0) >> 4);
		state.leftStick.y = p.sticks[2];
		state.rightStick.x = ((p.sticks[4] & 0x0F) << 4) | ((p.sticks[3] & 0xF0) >> 4);
		state.rightStick.y = p.sticks[5];
		
		updateCalibrationRange(state, cal);

		// Sets state.xinState's sticks
		calibrateToRange(state.leftStick, cal.left, cal.leftCenter, state.xinState.sThumbLX, state.xinState.sThumbLY);
		calibrateToRange(state.rightStick, cal.right, cal.rightCenter, state.xinState.sThumbRX, state.xinState.sThumbRY);

		vector<tuple<Button, bool>> buttons;
		pullButtonsFromByte(p.leftButtons, ButtonSource::Left, buttons);
		pullButtonsFromByte(p.rightButtons, ButtonSource::Right, buttons);
		pullButtonsFromByte(p.middleButtons, ButtonSource::Middle, buttons);

		for (auto pair : buttons) {
			if (std::get<1>(pair)) {
#ifdef _DEBUG
				std::cout << buttonToString(std::get<0>(pair)) << ' ';
#endif
				mapButtonToState(std::get<0>(pair), state);
			}
		}
	}
};

namespace Procon {

	void Controller::pollInput() {
		if (!device)
			return;

		auto dat = sendCommand(getInput, empty);
		if (!dat) {
			throw ControllerException("Error sending getInput command.");
		}
		if (dat.value()[0] != 0x30) {
			InputPacket p;
			memcpy(&p, dat.value().data(), sizeof(InputPacket));

			zeroPadState(padStatus);
			mapInputToState(p, calib, padStatus);
			
			DWORD err;
			if ((err = XOutputSetState(port, &padStatus.xinState)) != ERROR_SUCCESS) {
				std::string errMsg{ "XOutput Error: " };
				errMsg += std::to_string(err);
				throw ControllerException(errMsg);
			}
		}
		//updateStatus();
	}

	bool Controller::connected() const {
		return _connected;
	}
	uchar Controller::getPort() const {
		return port;
	}
	const ExpandedPadState& Controller::getState() const {
		return padStatus;
	}
	void Controller::setCalibrationCenter(const StickPoint &left, const StickPoint &right) {
		calib.leftCenter = left;
		calib.rightCenter = right;
	}
	void Controller::updateStatus() {
		if (clock::now() < lastStatus + std::chrono::milliseconds(100)) {
			return;
		}
		uchar vibrate{ 0 };
		uchar led{ 0 };
		uchar smallMotor{ 0 };
		uchar bigMotor{ 0 };
		XOutputGetState(port, &vibrate, &bigMotor, &smallMotor, &led);
		if (vibrate != 0) {
			sendRumble(bigMotor, 0);
			sendRumble(0, smallMotor);
		}
		array<uchar, 1> ledData{ static_cast<uchar>(0x1 << led) };
		sendSubcommand(0x1, ledCommand, ledData);
		lastStatus = clock::now();
	}

	Controller::exchangeArray Controller::sendRumble(uchar largeMotor, uchar smallMotor){
		std::array<uchar, 9> buf{
			static_cast<uchar>(rumbleCounter++ & 0xF),
			0x80_uc,
			0x00_uc,
			0x40_uc,
			0x40_uc,
			0x80_uc,
			0x00_uc,
			0x40_uc,
			0x40_uc
		};
		if (largeMotor != 0) {
			buf[1] = buf[5] = 0x08;
			buf[2] = buf[6] = largeMotor;
		}
		else if (smallMotor != 0) {
			buf[1] = buf[5] = 0x10;
			buf[2] = buf[6] = smallMotor;
		}
		return sendCommand(0x10, buf);
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
	unsigned char Procon::operator ""_uc(unsigned long long t) {
		return static_cast<unsigned char>(t);
	}
