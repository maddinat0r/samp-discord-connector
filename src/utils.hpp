#pragma once

#include <string>
#include <functional>
#include <json.hpp>


namespace utils
{
	inline bool TryDumpJson(nlohmann::json const &data, std::string &dest)
	{
		try
		{
			dest = data.dump();
		}
		catch (const json::type_error &e)
		{
			dest = e.what();
			return false;
		}
		return true;
	}


	inline bool IsValidJson(nlohmann::json const &data)
	{
		(void)data; // unused
		return true;
	}

	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, const char *key,
		const char *subkey, Ty&&... other);

	// useful when the key can be both Snowflake and null (e.g. voice channel on leave)
	template<typename... Ty>
	bool IsValidJson(nlohmann::json const &data, const char *key,
		nlohmann::json::value_t type, nlohmann::json::value_t type2, Ty&&... other)
	{
		auto jit = data.find(key);
		if (jit != data.end() && (jit->type() == type || jit->type() == type2))
			return IsValidJson(data, std::forward<Ty>(other)...);

		return false;
	}

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

	template <typename T>
	bool TryGetJsonValue(nlohmann::json const &data, T &dest)
	{
		try
		{
			dest = data.get<T>();
		}
		catch (nlohmann::json::type_error &)
		{
			return false;
		}
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
	bool TryGetJsonValue(nlohmann::json const &data, Td &dest, const char *key, const char *subkey, Ty&&... other)
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
