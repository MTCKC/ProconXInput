#include "XOutput.hpp"

#include <array>
#include <cstring>

namespace {

	using SetStateFuncPtr = DWORD(__cdecl *)(_In_ DWORD, _Out_ XINPUT_GAMEPAD*);
	using GetStateFuncPtr = DWORD(__cdecl *)(_In_ DWORD, _Out_ PBYTE, _Out_ PBYTE, _Out_ PBYTE, _Out_ PBYTE);
	using GetRealUserIndexFuncPtr = DWORD(__cdecl *)(_In_ DWORD, _Out_ DWORD*);
	using PlugInFuncPtr = DWORD(__cdecl *)(_In_ DWORD);
	using UnPlugFuncPtr = DWORD(__cdecl *)(_In_ DWORD);
	using UnPlugAllFuncPtr = DWORD(__cdecl *)();

	constexpr size_t buflen{ 80 };
	std::array<char, buflen> buf;

	// Load a function named 'funcName' from the Library pointed to by 'lib'
	// Throws an XOutputError on failure
	template <class T>
	T loadFunc(HMODULE lib, const char* funcName) {
		T out = reinterpret_cast<T>(GetProcAddress(lib, funcName));
		if (out == nullptr) {
			strncpy_s(buf.data(), buflen, "Unable to load function: ", buflen);
			strncpy_s(buf.data() + 26, buflen-26, funcName, buflen - 27);
			throw XOutput::XOutputError(buf.data());
		}
		return out;
	}


	// Simple external .dll library wrapper class for XOutput1_1.dll
	class XOutputLib {
		HMODULE dllHandle{ nullptr };
		SetStateFuncPtr setState{ nullptr };
		GetStateFuncPtr getState{ nullptr };
		GetRealUserIndexFuncPtr getUserIdx{ nullptr };
		PlugInFuncPtr plugIn{ nullptr };
		UnPlugFuncPtr unPlug{ nullptr };
		UnPlugAllFuncPtr unPlugAll{ nullptr };
		bool hasInit{ false };
	public:
		XOutputLib() = default;

		void init() {
			if (hasInit) return;
			hasInit = true;
			dllHandle = LoadLibrary("XOutput1_1.dll");
			if (dllHandle == nullptr) {
				throw XOutput::XOutputError("Unable to load XOutput1_1.dll");
			}
			setState = loadFunc<SetStateFuncPtr>(dllHandle, "XOutputSetState");
			getState = loadFunc<GetStateFuncPtr>(dllHandle, "XOutputGetState");
			getUserIdx = loadFunc<GetRealUserIndexFuncPtr>(dllHandle, "XOutputGetRealUserIndex");
			plugIn = loadFunc<PlugInFuncPtr>(dllHandle, "XOutputPlugIn");
			unPlug = loadFunc<UnPlugFuncPtr>(dllHandle, "XOutputUnPlug");
			unPlugAll = loadFunc<UnPlugAllFuncPtr>(dllHandle, "XOutputUnPlugAll");
			// all functions guaranteed to be not-null or exception is thrown
			
		}
		~XOutputLib() {
			if (unPlugAll != nullptr) {
				unPlugAll();
			}
			if (dllHandle != nullptr) {
				FreeLibrary(dllHandle);
				dllHandle = nullptr;
			}
		}
		inline DWORD SetState(DWORD userIndex, XINPUT_GAMEPAD* gamepad) {
			return setState(userIndex, gamepad);
		}
		inline DWORD GetState(DWORD userIndex, PBYTE vibrate, PBYTE largeMotor, PBYTE smallMotor, PBYTE led) {
			return getState(userIndex, vibrate, largeMotor, smallMotor, led);
		}
		inline DWORD GetRealUserIndex(DWORD userIndex, DWORD* realIndex) {
			return getUserIdx(userIndex, realIndex);
		}
		inline DWORD PlugIn(DWORD userIndex) {
			return plugIn(userIndex);
		}
		inline DWORD UnPlug(DWORD userIndex) {
			return unPlug(userIndex);
		}
		inline DWORD UnPlugAll() {
			return unPlugAll();
		}
	};
	XOutputLib lib;
}; // namespace



namespace XOutput {
	void XOutputInitialize() {
		lib.init();
	}
	DWORD XOutputSetState(DWORD userIdx, XINPUT_GAMEPAD* pad) {
		return lib.SetState(userIdx, pad);
	}
	DWORD XOutputGetState(DWORD userIdx, PBYTE vibrate, PBYTE lMotor, PBYTE sMotor, PBYTE led) {
		return lib.GetState(userIdx, vibrate, lMotor, sMotor, led);
	}
	DWORD XOutputGetRealUserIndex(DWORD userIndex, DWORD* realIndex) {
		return lib.GetRealUserIndex(userIndex, realIndex);
	}
	DWORD XOutputPlugIn(DWORD userIndex) {
		return lib.PlugIn(userIndex);
	}
	DWORD XOutputUnPlug(DWORD userIndex) {
		return lib.UnPlug(userIndex);
	}
	DWORD XOutputUnPlugAll() {
		return lib.UnPlugAll();
	}

	XOutputError::XOutputError(const std::string &what) : std::runtime_error(what) {}
	XOutputError::XOutputError(const char *what) : std::runtime_error(what) {}

}; // namespace XOutput