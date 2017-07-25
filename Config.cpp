#include "Config.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

using std::string;
using std::ifstream;
using std::stringstream;

using namespace Procon;

namespace {

	template<class T>
	using Store = Config::Store<T>;

	enum class ConfigType {
		Boolean,
		Integer,
		Float,
		Double,
		String
	};

	using ConfigInt = int32_t;

	const std::unordered_map<char, ConfigType> typemap{
		{ 'b', ConfigType::Boolean },
		{ 's', ConfigType::String },
		{ 'f', ConfigType::Float },
		{ 'd', ConfigType::Double },
		{ 'i', ConfigType::Integer }
	};

	template<class T>
	void readConfig(const string& line) {
		stringstream s{ line };
		stringstream name;
		T value;
		try {
			while (s.peek() != ' ' && s.peek() != '=') // Read up until space or equal sign
				name << static_cast<char>(s.get());
			while (s.peek() == ' ' || s.peek() == '=') // Skip everything that's a space or equal sign
				s.get();
			s >> value;
		}
		catch (const std::runtime_error &e) {
			stringstream error;
			error << "Error parsing config line : \n" << line << "\nException: " << e.what();
			throw ConfigError(error.str());
		}
		Config::store(name.str(), value);
	}

}; // namespace



void Config::readConfigFile(const string& filename) {
	ifstream file;
	file.open(filename);
	if (!file.good()) {
		string error{ "Unable to open config file " };
		error += filename;
		throw ConfigError(error);
	}
	string line;
	while (std::getline(file, line)) {
		if (line.size() == 0) continue;
		
		auto type = typemap.find(line[0]);
		if (type == typemap.end()) continue;

		switch (type->second) {
		case ConfigType::Boolean:
			readConfig<bool>(line);
			break;
			
		case ConfigType::Double:
			readConfig<double>(line);
			break;

		case ConfigType::Float:
			readConfig<float>(line);
			break;

		case ConfigType::Integer:
			readConfig<ConfigInt>(line);
			break;

		case ConfigType::String:
			readConfig<string>(line);
			break;
		} // switch (type->second)
	} // while (std::getline(file, line))

}// readConfigFile()

ConfigError::ConfigError(const string& what) : std::runtime_error(what) {}
ConfigError::ConfigError(const char* what) : std::runtime_error(what) {}
