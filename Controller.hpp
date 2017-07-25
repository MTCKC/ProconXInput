#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <chrono>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>

#include "Common.hpp"
#include "hidapi.h"

namespace Procon {

	constexpr size_t exchangeLen{ 0x400 };
	struct AxisRange {
		uchar min;
		uchar max;
	};
	struct StickRange {
		AxisRange x;
		AxisRange y;
	};
	struct StickPoint {
		uchar x;
		uchar y;
	};
	struct CalibrationData {
		StickRange left;
		StickRange right;
		StickPoint leftCenter;
		StickPoint rightCenter;
	};
	void SetDefaultCalibration(CalibrationData &dat);
	struct HIDCloser {
		void operator()(hid_device *ptr);
	};
	struct ExpandedPadState {
		XINPUT_GAMEPAD xinState;
		StickPoint leftStick;
		StickPoint rightStick;
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
		clock::time_point lastStatus{ clock::now() };
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
		void setCalibrationCenter(const StickPoint &left, const StickPoint &right);
	private:

		void updateStatus();
		
		using exchangeArray = std::optional<std::array<uchar, exchangeLen>>;

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
				0x00_uc,
				0x01_uc,
				0x40_uc,
				0x40_uc,
				0x00_uc,
				0x01_uc,
				0x40_uc,
				0x40_uc,
				subcommand
			};
			if (len > 0) {
				memcpy(buf.data() + 10, data.data(), len);
			}
			return sendCommand(command, buf);
		}


		exchangeArray sendRumble(uchar largeMotor, uchar smallMotor);

	};

	class ControllerException : public std::runtime_error {
	public:
		explicit ControllerException(const std::string& what);
		explicit ControllerException(const char* what);
	};

};
