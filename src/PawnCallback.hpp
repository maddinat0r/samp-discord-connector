#pragma once

#include "CSingleton.hpp"
#include "sdk.hpp"

#include <string>
#include <unordered_set>
#include <memory>


class PawnCallbackManager : public CSingleton<PawnCallbackManager>
{
	friend class CSingleton<PawnCallbackManager>;

private: //constructor / destructor
	PawnCallbackManager() = default;
	~PawnCallbackManager() = default;

private: //variables
	std::unordered_set<AMX *> m_AmxInstances;

private: // functions
	// cell
	template<typename... Tv>
	void PushArgs(AMX *amx, cell &push_addr, cell value, Tv&&... other_values)
	{
		PushArgs(amx, push_addr, std::forward<Tv>(other_values)...);
		amx_Push(amx, value);
	}

	// C string
	template<typename... Tv>
	void PushArgs(AMX *amx, cell &push_addr, const char *value, Tv&&... other_values)
	{
		PushArgs(amx, push_addr, std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushString(amx, &tmp_addr, nullptr, value, 0, 0);

		if (push_addr < 0)
			push_addr = tmp_addr;
	}

	// std string
	template<typename... Tv>
	void PushArgs(AMX *amx, cell &push_addr, std::string const &value, Tv&&... other_values)
	{
		PushArgs(amx, push_addr, std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushString(amx, &tmp_addr, nullptr, value.c_str(), 0, 0);

		if (push_addr < 0)
			push_addr = tmp_addr;
	}

	// cell array
	template<size_t N, typename... Tv>
	void PushArgs(AMX *amx, cell &push_addr, const cell(&array)[N], Tv&&... other_values)
	{
		PushArgs(amx, push_addr, std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushArray(amx, &tmp_addr, nullptr, array, N);

		if (push_addr < 0)
			push_addr = tmp_addr;
	}

	// reference
	template<size_t N, typename... Tv>
	void PushArgs(AMX *amx, cell &push_addr, cell *value, Tv&&... other_values)
	{
		PushArgs(amx, push_addr, std::forward<Tv>(other_values)...);
		amx_PushAddress(amx, value);
	}

	void PushArgs(AMX *amx, cell &push_addr)
	{ }

	void Execute(const char *name, AMX *amx, int cb_idx, cell args_push_addr);

public: //functions
	inline void AddAmx(AMX *amx)
	{
		m_AmxInstances.insert(amx);
	}
	inline void RemoveAmx(AMX *amx)
	{
		m_AmxInstances.erase(amx);
	}

	template<size_t N, typename... Tv>
	void Call(const char (&name)[N], Tv&&... params)
	{
		// 5 = strlen('DCC_') + 1
		static_assert(N > 5, "callback name too short");

		for (AMX *amx : m_AmxInstances)
		{
			int cb_idx = -1;
			if (amx_FindPublic(amx, name, &cb_idx) != AMX_ERR_NONE)
				continue;

			cell push_addr = -1;
			PushArgs(amx, push_addr, std::forward<Tv>(params)...);

			Execute(name, amx, cb_idx, push_addr);
		}
	}
};
