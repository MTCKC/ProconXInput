#pragma once

#include <memory>
#include <stdexcept>
#include <string>

namespace Procon {
	
	class CerberusError : public std::runtime_error {
	public:
		CerberusError(const std::string & what);
		CerberusError(const char* what);
	};

	// RAII wrapper around HidCerberus.
	// Create one, then call init() to open HidCerberus. Cleanup is when the
	// object is destructed.
	// Throws Procon::CerberusError from init() only.
	struct CerberusImpl;
	class Cerberus {
		std::unique_ptr<CerberusImpl> impl;
	public:
		Cerberus();
		// No copying;
		Cerberus(const Cerberus&) = delete;
		Cerberus& operator=(const Cerberus&) = delete;
		// Moving OK
		Cerberus(Cerberus &&);
		Cerberus& operator=(Cerberus&&);
		~Cerberus();

		void init();
	};
};