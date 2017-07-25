#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace Procon {

	class Config {
	public:
		template<class T>
		using Store = std::unordered_map<std::string, T>;

	private:
		template<class T>
		static Store<T>& getStore() {
			static Store<T> store;
			return store;
		}

	public:
		Config() = delete;
		Config(const Config&) = delete;
		Config(Config&&) = delete;
		Config& operator=(const Config&) = delete;
		Config& operator=(Config&&) = delete;

		static void readConfigFile(const std::string& filename);

		template<class T>
		static std::optional<T> get(const std::string& name) {
			Store<T>& store = getStore<T>();
			auto it = store.find(name);
			if (it != store.end())
				return it->second;
			return {};
		}

		template<class T>
		static void store(const std::string& name, const T& value) {
			getStore<T>().insert({ name, value });
		}


	}; // class Config

	struct ConfigError : std::runtime_error {
		ConfigError(const std::string& what);
		ConfigError(const char* what);
	};

} // namespace Procon
