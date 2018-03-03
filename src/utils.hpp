#pragma once

#include <string>
#include <functional>
#include <json.hpp>


namespace utils
{
	namespace _internal
	{
		inline bool IsValidJson(const nlohmann::json &data)
		{
			return true;
		}

		template<typename... Ty>
		bool IsValidJson(nlohmann::json const &data, std::string const &key,
			nlohmann::json::value_t type, Ty&&... other)
		{
			auto &jit = data.find(key);
			if (jit != data.end() && jit->type() == type)
				return IsValidJson(data, std::forward<Ty>(other)...);

			return false;
		}

		// for json objects
		template<typename... Ty>
		bool IsValidJson(nlohmann::json const &data, std::string const &key,
			std::string const &subkey, Ty&&... other)
		{
			auto &jit = data.find(key);
			if (jit != data.end() && jit->type() == nlohmann::json::value_t::object)
				return IsValidJson(*jit, subkey, std::forward<Ty>(other)...);

			return false;
		}
	}

	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, Ty&&... other)
	{
		return _internal::IsValidJson(data, std::forward<Ty>(other)...);
	}


	namespace _internal
	{
		inline bool GetJsonValue(nlohmann::json const &data, bool &dest)
		{
			if (data.type() != nlohmann::json::value_t::boolean)
				return false;

			dest = data.get<bool>();
			return true;
		}

		inline bool GetJsonValue(nlohmann::json const &data, std::string &dest)
		{
			if (data.type() != nlohmann::json::value_t::string)
				return false;

			dest = data.get<std::string>();
			return true;
		}

		inline bool GetJsonValue(nlohmann::json const &data, float &dest)
		{
			if (data.type() != nlohmann::json::value_t::number_float)
				return false;

			dest = data.get<float>();
			return true;
		}

		template<typename Td>
		typename std::enable_if<std::is_integral<Td>::value && std::is_unsigned<Td>::value, bool>::type 
			GetJsonValue(nlohmann::json const &data, Td &dest)
		{
			if (data.type() != nlohmann::json::value_t::number_unsigned)
				return false;

			dest = data.get<Td>();
			return true;
		}

		template<typename Td>
		typename std::enable_if<std::is_integral<Td>::value && std::is_signed<Td>::value, bool>::type 
			GetJsonValue(nlohmann::json const &data, Td &dest)
		{
			if (data.type() != nlohmann::json::value_t::number_integer)
				return false;

			dest = data.get<Td>();
			return true;
		}


		template<typename... Ty, typename Td>
		bool TryGetJsonValue(nlohmann::json const &data, Td &dest,
			std::string const &key)
		{
			auto &jit = data.find(key);
			if (jit != data.end())
				return GetJsonValue(*jit, dest);

			return false;
		}

		// for json object
		template<typename... Ty, typename Td>
		bool TryGetJsonValue(nlohmann::json const &data, Td &dest,
			std::string const &key, std::string const &subkey, Ty&&... other)
		{
			auto &jit = data.find(key);
			if (jit != data.end() && jit->type() == nlohmann::json::value_t::object)
			{
				return TryGetJsonValue(*jit, dest, subkey,
					std::forward<Ty>(other)...);
			}

			return false;
		}
	}

	// validates and fetches value if valid
	template<typename... Ty, typename Td>
	bool TryGetJsonValue(nlohmann::json const &data, Td &dest, Ty&&... other)
	{
		return _internal::TryGetJsonValue(data, dest, std::forward<Ty>(other)...);
	}
}
