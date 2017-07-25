#ifndef NO_CERBERUS
#include "Cerberus.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <stdexcept>

namespace Procon {

	CerberusError::CerberusError(const std::string &what) : std::runtime_error(what) {}
	CerberusError::CerberusError(const char *what) : std::runtime_error(what) {}

	using HidGuardianFunc = BOOL(WINAPI*)();

	struct Cerberus::CerberusImpl {
		HMODULE library{ nullptr };
		HidGuardianFunc open{ nullptr };
		HidGuardianFunc close{ nullptr };
	};

	Cerberus::Cerberus() :impl(new CerberusImpl()) {
	}
	Cerberus::~Cerberus() {
		if (impl->close != nullptr) {
			impl->close();
			impl->close = nullptr;
		}
		if (impl->library != nullptr) {
			FreeLibrary(impl->library);
			impl->library = nullptr;
		}
	}

	// Move ctor and operator= defaulted here to allow pimpl to work with shared_ptr, don't use '= default;' in header
	Cerberus::Cerberus(Cerberus &&) = default;
	Cerberus& Cerberus::operator=(Cerberus &&) = default;

	void Cerberus::init() {
		impl->library = LoadLibrary(L"HidCerberus.Lib.dll");
		if (impl->library == nullptr) {
			throw CerberusError("Unable to load HidCerberus.Lib.dll");
		}

		impl->open = reinterpret_cast<HidGuardianFunc>(GetProcAddress(impl->library, "HidGuardianOpen"));
		impl->close = reinterpret_cast<HidGuardianFunc>(GetProcAddress(impl->library, "HidGuardianClose"));

		if (impl->open == nullptr) {
			throw CerberusError("Unable to load function HidGuardianOpen");
		}
		if (impl->close == nullptr) {
			throw CerberusError("Unable to load function HidGuardianClose");
		}
		if (!impl->open()) {
			throw CerberusError("Error connecting to the Cerberus service");
		}
		impl->open = nullptr;
	}
};
#endif
