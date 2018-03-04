#pragma once

#include <string>
#include <functional>
#include <json.hpp>


namespace utils
{
	inline bool IsValidJson(nlohmann::json const &data)
	{
		return true;
	}

	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, const char *key,
		const char *subkey, Ty&&... other);

	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, const char *key,
		nlohmann::json::value_t type, Ty&&... other)
	{
		auto jit = data.find(key);
		if (jit != data.end() && jit->type() == type)
			return IsValidJson(data, std::forward<Ty>(other)...);

		return false;
	}

	// for json objects
	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, const char *key,
		const char *subkey, Ty&&... other)
	{
		auto jit = data.find(key);
		if (jit != data.end() && jit->type() == nlohmann::json::value_t::object)
			return IsValidJson(*jit, subkey, std::forward<Ty>(other)...);

		return false;
	}




	inline bool TryGetJsonValue(nlohmann::json const &data, bool &dest)
	{
		if (data.type() != nlohmann::json::value_t::boolean)
			return false;

		dest = data.get<bool>();
		return true;
	}

	inline bool TryGetJsonValue(nlohmann::json const &data, std::string &dest)
	{
		if (data.type() != nlohmann::json::value_t::string)
			return false;

		dest = data.get<std::string>();
		return true;
	}

	inline bool TryGetJsonValue(nlohmann::json const &data, float &dest)
	{
		if (data.type() != nlohmann::json::value_t::number_float)
			return false;

		dest = data.get<float>();
		return true;
	}

	template<typename Td>
	typename std::enable_if<std::is_integral<Td>::value && std::is_unsigned<Td>::value, bool>::type
		TryGetJsonValue(nlohmann::json const &data, Td &dest)
	{
		if (data.type() != nlohmann::json::value_t::number_unsigned)
			return false;

		dest = data.get<Td>();
		return true;
	}

	template<typename Td>
	typename std::enable_if<std::is_integral<Td>::value && std::is_signed<Td>::value, bool>::type
		TryGetJsonValue(nlohmann::json const &data, Td &dest)
	{
		if (data.type() != nlohmann::json::value_t::number_integer)
			return false;

		dest = data.get<Td>();
		return true;
	}

	template<typename... Ty, typename Td>
	bool TryGetJsonValue(nlohmann::json const &data, Td &dest, const char *key)
	{
		auto jit = data.find(key);
		if (jit != data.end())
			return TryGetJsonValue(*jit, dest);

		return false;
	}

	// for json object
	template<typename... Ty, typename Td>
	bool TryGetJsonValue(nlohmann::json const &data, Td &dest, const char *key,
		const char *subkey, Ty&&... other)
	{
		auto jit = data.find(key);
		if (jit != data.end() && jit->type() == nlohmann::json::value_t::object)
		{
			return TryGetJsonValue(*jit, dest, subkey,
				std::forward<Ty>(other)...);
		}

		return false;
	}
}
