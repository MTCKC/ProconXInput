#pragma once

#include <memory>
#include <optional>
#include <stdexcept>

#include <ViGEmUM.h>

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
		VIGEM_TARGET vController;
		std::unique_ptr<hid_device, HIDCloser> device;

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
			std::array<uchar, 10 + len> buf;
			buf.fill(0);
			static uchar rumbleCounter = 0;
			buf[0] = ++rumbleCounter & 0xF;
			buf[1] = 0x00;
			buf[2] = 0x01;
			buf[3] = 0x40;
			buf[4] = 0x40;
			buf[5] = 0x00;
			buf[6] = 0x01;
			buf[7] = 0x40;
			buf[8] = 0x40;
			buf[9] = subcommand;
			memcpy(buf.data() + 10, data.data(), len);
			return sendCommand(command, buf);
		}
	};

	class ControllerException : public std::runtime_error {
	public:
		explicit ControllerException(const std::string& what);
		explicit ControllerException(const char* what);
	};

};