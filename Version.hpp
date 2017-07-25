#pragma once
#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace Procon {
	using VerType = size_t;
	enum class ReleaseType {
		Alpha,
		Beta,
		Release
	};

	struct Version {
		VerType major;
		VerType minor;
		VerType patch;
		ReleaseType type;
		VerType typeSubver;
	};
	constexpr char ProgramName[] = "ProconXInput";

#ifdef _DEBUG
	constexpr char BuildType[] = "Debug Build";
#else
	constexpr char BuildType[] = "Release Build";
#endif

#if _WIN64
	constexpr char Platform[] = "64-bit";
#elif _WIN32
	constexpr char Platform[] = "32-bit";
#else
	static_assert(false, "Platform unable to be determined.");
#endif

	constexpr Version ProgramVersion{ 0, 1, 0, ReleaseType::Alpha, 3 };
	std::ostream& operator<<(std::ostream& o, const ReleaseType& vt);
	std::ostream& operator<<(std::ostream& o, const Version& v);
};
