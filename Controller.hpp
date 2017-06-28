#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <chrono>
#include <thread>

#define NOMINMAX
#include <Windows.h>
#include <XOutput.h>

#include "Common.hpp"
#include "hidapi.h"

namespace Procon {

	constexpr size_t exchangeLen{ 0x400 };

	struct HIDCloser {
		void operator()(hid_device *ptr);
	};

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
		clock::time_point lastCommand{ clock::now() };
		uchar currentLed{ 0 };
		uchar largeMotor{ 0 };
		uchar smallMotor{ 0 };
	public:
		Controller();
		Controller(Controller &&);
		Controller(const Controller&) = delete;
		Controller& operator=(const Controller&) = delete;
		Controller& operator=(Controller &&);
		~Controller();

		void openDevice(hid_device_info *dev);
		void pollInput();

		bool connected() const;
		
	private:
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
			memcpy(buf.data() + 0x9, data.data(), len);
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
			memcpy(buf.data() + 10, data.data(), len);
			lastCommand = clock::now();
			return sendCommand(command, buf);
		}


		template<size_t len>
		exchangeArray sendRumble(uchar largeMotor, uchar smallMotor) {
			while (clock::now() < lastCommand + std::chrono::milliseconds(100)) {
				std::this_thread::yield();
			}
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
			lastCommand = clock::now();
			return sendCommand(0x10, buf);
		}

	};

	class ControllerException : public std::runtime_error {
	public:
		explicit ControllerException(const std::string& what);
		explicit ControllerException(const char* what);
	};

};