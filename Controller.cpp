#include "Controller.hpp"

#include <array>
#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif
#include <string>
#include <limits>

#include "hidapi.h"
#include "XOutput.hpp"
#include "Config.hpp"

using namespace XOutput;


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

#pragma pack(push, 1)
	struct InputPacket {
		uint8_t header;
		uint8_t timer;
		uint8_t info;
		uint8_t rightButtons;
		uint8_t middleButtons;
		uint8_t leftButtons;
		uint8_t sticks[6];
		uint8_t vibrator;
		uint16_t accel_1[3];
		uint16_t gyro_1[3];
		uint16_t accel_2[3];
		uint16_t gyro_2[3];
		uint16_t accel_3[3];
		uint16_t gyro_3[3];
	};
#pragma pack(pop)

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

	using std::vector;
	using std::tuple;
	using std::array;

	using namespace Procon;

	const std::array<Button, 8> JoyconLBitmap =
	{
		Button::DPadDown,
		Button::DPadUp,
		Button::DPadRight,
		Button::DPadLeft,
		Button::None,
		Button::None,
		Button::L,
		Button::LZ
	};

	const std::array<Button, 8> JoyconRBitmap = {
		Button::Y,
		Button::X,
		Button::B,
		Button::A,
		Button::None,
		Button::None,
		Button::R,
		Button::RZ
	};

	const std::array<Button, 8> JoyconMidBitmap = {
		Button::Minus,
		Button::Plus,
		Button::RStick,
		Button::LStick,
		Button::Home,
		Button::Share,
		Button::None,
		Button::None
	};

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

	constexpr unsigned short buttonToReportBits(Button b) {
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

#ifdef _DEBUG
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

	const std::string& buttonToString(Button b) {
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
#endif //#ifdef _DEBUG

	void mapInputToState(const InputPacket &p, CalibrationData &cal, ExpandedPadState &state) {
		auto uint16ToInt16 = [](uint16_t a) {return *(int16_t*)(&a); };

		state.leftStick.x = p.sticks[0] | ((p.sticks[1] & 0xF) << 8);
		state.leftStick.y = (p.sticks[1] >> 4 | p.sticks[2] << 4);
		state.rightStick.x = p.sticks[3] | ((p.sticks[4] & 0xF) << 8);
		state.rightStick.y = (p.sticks[4] >> 4 | p.sticks[5] << 4);

		state.accel.x = uint16ToInt16(p.accel_1[0]) * cal.motionSensor.accelCoeff[0];
		state.accel.y = uint16ToInt16(p.accel_1[1]) * cal.motionSensor.accelCoeff[1];
		state.accel.z = uint16ToInt16(p.accel_1[2]) * cal.motionSensor.accelCoeff[2];

		state.gyro.x = uint16ToInt16(p.gyro_1[0]) * cal.motionSensor.gyroCoeff[0];
		state.gyro.y = uint16ToInt16(p.gyro_1[1]) * cal.motionSensor.gyroCoeff[1];
		state.gyro.z = uint16ToInt16(p.gyro_1[2]) * cal.motionSensor.gyroCoeff[2];

		// Sets state.xinState's sticks
		calibrateToRange(state.leftStick, cal.left, cal.leftCenter, state.xinState.sThumbLX, state.xinState.sThumbLY);
		calibrateToRange(state.rightStick, cal.right, cal.rightCenter, state.xinState.sThumbRX, state.xinState.sThumbRY);


		pullButtonsFromByte(p.leftButtons, ButtonSource::Left, state.buttons);
		pullButtonsFromByte(p.rightButtons, ButtonSource::Right, state.buttons);
		pullButtonsFromByte(p.middleButtons, ButtonSource::Middle, state.buttons);

		for (auto pair : state.buttons) {
			if (std::get<1>(pair)) {
#ifdef _DEBUG
				std::cout << buttonToString(std::get<0>(pair)) << ' ';
#endif
				mapButtonToState(std::get<0>(pair), state);
			}
		}
	}

	unsigned char operator ""_uc(unsigned long long t) {
		return static_cast<unsigned char>(t);
	}
}; //namespace

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
		for (auto& button : state.buttons)
		{
			std::get<1>(button) = false;
		}
		state.xinState = { 0 };
		state.leftStick = { 0 };
		state.rightStick = { 0 };
		state.accel = { 0, 0, 0 };
		state.gyro = { 0, 0, 0 };
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

	void Controller::openDevice(hid_device_info *dev) {
		using namespace std::this_thread;
		using namespace std::chrono;

		if (dev == nullptr)
			throw ControllerException("Unable to open controller device: dev was nullptr.");
		if (dev->product_id != Procon_ID)
			throw ControllerException("Unable to open controller device: product id was not a Switch Pro Controller.");
		if (wcscmp(dev->serial_number, L"000000000001") != 0)
			throw ControllerException("Device is not connected via USB");

		device.reset(hid_open_path(dev->path));
		if (!device)
			throw ControllerException("Unable to open controller device: device path could not be opened.");

		// Start handshake
		if (!exchange(handshake)) {
			throw ControllerException("Handshake failed.");
		}
		exchange(switchBaudrate);
		exchange(handshake);
		exchange(HIDOnlyMode);

		//sendSubcommand(0x1, rumbleCommand, enable);
		// Set report mode to 0x30 (we now get the same data as in the bluetooth case)
		std::array<uchar, 2> report{ 0x30, 0x00 };
		sendSubcommand(0x01, 0x03, report);
		sendSubcommand(0x01, imuDataCommand, enable);
		sendSubcommand(0x01, ledCommand, led);

		// Read calibration data (use factory calibration for now)
		readAnalogStickCalibrationData();
		readMotionSensorCalibrationData();

		if (XOutputPlugIn(port) != ERROR_SUCCESS) {
			device.reset(nullptr);
			throw ControllerException("Unable to plugin XOutput controller.");
		}
		_connected = true;
		sleep_for(milliseconds(100));
		//updateStatus();
	}

	void Controller::readAnalogStickCalibrationData()
	{
		auto factory_stick_cal = getSpiData(0x603D, 0x12).value();

		calib.leftCenter.x = (factory_stick_cal[4] << 8) & 0xF00 | factory_stick_cal[3];
		calib.leftCenter.y = (factory_stick_cal[5] << 4) | (factory_stick_cal[4] >> 4);
		calib.left.x.min = calib.leftCenter.x - ((factory_stick_cal[7] << 8) & 0xF00 | factory_stick_cal[6]);
		calib.left.x.max = calib.leftCenter.x + ((factory_stick_cal[1] << 8) & 0xF00 | factory_stick_cal[0]);
		calib.left.y.min = calib.leftCenter.y - ((factory_stick_cal[8] << 4) | (factory_stick_cal[7] >> 4));
		calib.left.y.max = calib.leftCenter.y + ((factory_stick_cal[2] << 4) | (factory_stick_cal[1] >> 4));

		calib.rightCenter.x = (factory_stick_cal[10] << 8) & 0xF00 | factory_stick_cal[9];
		calib.rightCenter.y = (factory_stick_cal[11] << 4) | (factory_stick_cal[10] >> 4);
		calib.right.x.min = calib.rightCenter.x - ((factory_stick_cal[13] << 8) & 0xF00 | factory_stick_cal[12]);
		calib.right.x.max = calib.rightCenter.x + ((factory_stick_cal[16] << 8) & 0xF00 | factory_stick_cal[15]);
		calib.right.y.min = calib.rightCenter.y - ((factory_stick_cal[14] << 4) | (factory_stick_cal[13] >> 4));
		calib.right.y.max = calib.rightCenter.y + ((factory_stick_cal[17] << 4) | (factory_stick_cal[16] >> 4));
	}

	void Controller::readMotionSensorCalibrationData()
	{
		auto factory_sensor_cal = getSpiData(0x6020, 0x18).value();
		auto uint16ToInt16 = [](uint16_t a) {return *(int16_t*)(&a); };
		array<int16_t, 3> accel_cal
		{
			uint16ToInt16(factory_sensor_cal[0x00] | factory_sensor_cal[0x01] << 8),
			uint16ToInt16(factory_sensor_cal[0x02] | factory_sensor_cal[0x03] << 8),
			uint16ToInt16(factory_sensor_cal[0x04] | factory_sensor_cal[0x05] << 8)
		};

		array<int16_t, 3> gyro_cal
		{
			uint16ToInt16(factory_sensor_cal[0x0C] | factory_sensor_cal[0x0D] << 8),
			uint16ToInt16(factory_sensor_cal[0x0E] | factory_sensor_cal[0x0F] << 8),
			uint16ToInt16(factory_sensor_cal[0x10] | factory_sensor_cal[0x11] << 8)
		};

		calib.motionSensor.accelCoeff = {
			(float)(1.0 / (float)(16384 - accel_cal[0])) * 4.0f/*  * 9.8f*/,
			(float)(1.0 / (float)(16384 - accel_cal[1])) * 4.0f/*  * 9.8f*/,
			(float)(1.0 / (float)(16384 - accel_cal[2])) * 4.0f/*  * 9.8f*/
		};

		calib.motionSensor.gyroCoeff = {
			(float)(936.0 / (float)(13371 - gyro_cal[0])/* * 0.01745329251994*/),
			(float)(936.0 / (float)(13371 - gyro_cal[1])/* * 0.01745329251994*/),
			(float)(936.0 / (float)(13371 - gyro_cal[2])/* * 0.01745329251994*/)
		};
	}

	void Controller::pollInput() {
		if (!device)
			return;

		//auto dat = sendCommand(getInput, empty);
		auto dat = receive(200);
		if (!dat)
		{
			return;
		}

		if (dat.value()[0] == 0x30 || dat.value()[0] == 0x31
			|| dat.value()[0] == 0x32 || dat.value()[0] == 0x33)
		{
			padStatus.lastStatus = clock::now();

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

	Controller::exchangeArray Controller::receive(int timeout)
	{
		std::array<uchar, exchangeLen> ret;
		ret.fill(0);
		int retLen = hid_read_timeout(device.get(), ret.data(), exchangeLen, timeout);
		if (retLen == 0)
			return {};
		return ret;
	}

	Controller::exchangeArray Controller::sendRumble(uchar largeMotor, uchar smallMotor) {
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

	Controller::exchangeArray Controller::getSpiData(uint32_t offset, const uint8_t readLen)
	{
		std::array<uint8_t, 0x100> buf;
		while (true)
		{
			buf.fill(0);
			*(uint32_t*)(buf.data()) = offset;
			*(uint8_t*)(buf.data() + 4) = readLen;
			auto res = sendSubcommand(0x01, 0x10, buf);
			//if (res && (*(uint16_t*)(res.value().data() + 0xD) == 0x1090) && (*(uint32_t*)(res.value().data() + 0xF) == offset)) // For bluetooth
			if (res && (*(uint16_t*)(res.value().data() + 0x17) == 0x1090) && (*(uint32_t*)(res.value().data() + 0x19) == offset))
			{
				std::array<uchar, exchangeLen> ret;
				ret.fill(0);
				//memcpy(ret.data(), res.value().data() + 0x14, readLen); // For bluetooth
				memcpy(ret.data(), res.value().data() + 0x1E, readLen);
				return ret;
			}
		}
		return {};
	}

	ControllerException::ControllerException(const std::string& what) : runtime_error(what) {}
	ControllerException::ControllerException(const char* what) : runtime_error(what) {}
};
