#include "Version.hpp"

namespace Procon {
	std::ostream& operator<<(std::ostream& o, const ReleaseType& vt) {
		switch (vt) {
		case ReleaseType::Alpha:
			o << "Alpha";
			break;
		case ReleaseType::Beta:
			o << "Beta";
			break;
		case ReleaseType::Release:
			o << "Release";
			break;
		}
		return o;
	}
	std::ostream& operator<<(std::ostream& o, const Version& v) {
		o << "Version " << v.major << '.' << v.minor << '.' << v.patch;
		if (v.type != ReleaseType::Release) {
			o << "-" << v.type << v.typeSubver;
		}
		return o;
	}
};
