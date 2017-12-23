#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <vector>
#include <array>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>

#include "Common.hpp"
#include "hidapi.h"

namespace Procon {

	constexpr size_t exchangeLen{ 0x400 };

  struct AccelData {
    float x;
    float y;
    float z;
  };

  struct GyroData {
    float x;
    float y;
    float z;
  };

  struct AxisRange {
		uint16_t min;
    uint16_t max;
	};
	
  struct StickRange {
		AxisRange x;
		AxisRange y;
	};
	
  struct StickPoint {
    uint16_t x;
    uint16_t y;
	};

  struct SensorCalibration {
    std::array<float, 3> accelCoeff;
    std::array<float, 3> gyroCoeff;
  };
	
  struct CalibrationData {
		StickRange left;
		StickRange right;
		StickPoint leftCenter;
		StickPoint rightCenter;
    SensorCalibration motionSensor;
	};

	void SetDefaultCalibration(CalibrationData &dat);
	
  struct HIDCloser {
		void operator()(hid_device *ptr);
	};
	
  struct ExpandedPadState {
		XINPUT_GAMEPAD xinState;
    std::vector<std::tuple<Button, bool>> buttons;
		StickPoint leftStick;
		StickPoint rightStick;
    AccelData accel;
    GyroData gyro;
    std::chrono::steady_clock::time_point lastStatus;
		bool sharePressed;
	};
	
  void zeroPadState(ExpandedPadState &state);
	
  // Switch Procon class.
	// Create, then call openDevice(hid_device_info) to initialize.
	// Call pollInput() to send input to ViGEm, such as in a main loop.
	// Cleanup is automatic when the object is destroyed.
	// Throws Procon::Controller exceptions from openDevice.
	class Controller {
		bool _connected;
		std::unique_ptr<hid_device, HIDCloser> device;
		uchar rumbleCounter{ 0 };
		using clock = std::chrono::steady_clock;
    clock::time_point lastStatus;
		uchar port{ 0 };
		ExpandedPadState padStatus{};
		CalibrationData calib;
	public:
		Controller(uchar port);
		Controller(Controller &&);
		Controller(const Controller&) = delete;
		Controller& operator=(const Controller&) = delete;
		Controller& operator=(Controller &&);
		~Controller();

		void openDevice(hid_device_info *dev);
		void pollInput();

		bool connected() const;
		uchar getPort() const;
		const ExpandedPadState& getState() const;
	private:

		void updateStatus();
    void readAnalogStickCalibrationData();
    void readMotionSensorCalibrationData();
		
		using exchangeArray = std::optional<std::array<uchar, exchangeLen>>;

    exchangeArray receive(int timeout);

		template<size_t len>
		exchangeArray exchange(std::array<uchar, len> const &data) {
			if (!device) return {};

			if (hid_write(device.get(), data.data(), len) < 0) {
				return {};
			}
			std::array<uchar, exchangeLen> ret;
			ret.fill(0);
			hid_read(device.get(), ret.data(), exchangeLen);
			return ret;
		}

		template<size_t len>
		exchangeArray sendCommand(uchar command, std::array<uchar, len> const &data) {
			std::array<uchar, len + 0x9> buf;
			buf.fill(0);
			buf[0x0] = 0x80;
			buf[0x1] = 0x92;
			buf[0x3] = 0x31;
			buf[0x8] = command;
			if (len > 0) {
				memcpy(buf.data() + 0x9, data.data(), len);
			}
			return exchange(buf);
		}


		template<size_t len>
		exchangeArray sendSubcommand(uchar command, uchar subcommand, std::array<uchar, len> const& data) {
			std::array<uchar, 10 + len> buf
			{ 
				static_cast<uchar>(rumbleCounter++ & 0xF),
        // For command 0x01 this is the rumble data
				0x00_uc,
				0x01_uc,
				0x40_uc,
				0x40_uc,
				0x00_uc,
				0x01_uc,
				0x40_uc,
				0x40_uc,
        // Rumble data end
        // Now comes the additional subcommand if using 0x01 instead of 0x10 for rumble data
				subcommand
			};
			if (len > 0) {
				memcpy(buf.data() + 10, data.data(), len);
			}
			return sendCommand(command, buf);
		}

    exchangeArray getSpiData(uint32_t offset, const uint8_t readLen);
		exchangeArray sendRumble(uchar largeMotor, uchar smallMotor);
	};

	class ControllerException : public std::runtime_error {
	public:
		explicit ControllerException(const std::string& what);
		explicit ControllerException(const char* what);
	};

};
