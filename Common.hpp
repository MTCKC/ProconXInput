#pragma once

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
		MaxButtons
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

};
