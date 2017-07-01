#pragma once

#include <stdexcept>
#include <array>
#include <string>

namespace Procon {

	using uchar = unsigned char;

	// RAII function object
	template<class T>
	class ScopedFunction {
		T t;
	public:
		explicit ScopedFunction(T&& f) :t(f) {}
		explicit ScopedFunction(const T& f) :t(f) {}
		~ScopedFunction() {
			t();
		}
		ScopedFunction& operator=(T&& f) { t = f; }
		ScopedFunction& operator=(const T& f) { t = f; }
	};
	template<class T>
	ScopedFunction<T> make_scoped(T f) {
		return ScopedFunction<T>(std::move(f));
	}

	enum class Button {
		None,
		DPadUp,
		DPadDown,
		DPadRight,
		DPadLeft,
		A,
		B,
		X,
		Y,
		Plus,
		Minus,
		L,
		LZ,
		R,
		RZ,
		LStick,
		RStick,
		Home,
		Share,
	};

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

	enum class ButtonSource {
		Left,
		Right,
		Middle
	};

	constexpr int JoyconL_ID = 0x2006;
	constexpr int JoyconR_ID = 0x2007;
	constexpr int Procon_ID = 0x2009;
	constexpr int JoyconGrip_ID = 0x200e;
	constexpr int NintendoID = 0x057E;

	const std::string& buttonToString(Button b); // In Controller.cpp

	unsigned char operator ""_uc(unsigned long long t); // In Controller.cpp

};