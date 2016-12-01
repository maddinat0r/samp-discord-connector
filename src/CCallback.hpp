#pragma once

#include "CSingleton.hpp"
#include "sdk.hpp"

#include <string>
#include <unordered_set>
#include <memory>

using std::string;
using std::unordered_set;

#include "CError.hpp"


using Callback_t = std::shared_ptr<class CCallback>;

class CCallback
{
public: //type definitions
	enum class Error
	{
		NONE,
		INVALID_AMX,
		INVALID_PARAMETERS,
		INVALID_PARAM_COUNT,
		INVALID_FORMAT_SPECIFIER,
		EMPTY_NAME,
		NOT_FOUND,
		EXPECTED_ARRAY_SIZE,
		INVALID_ARRAY_SIZE,
		NO_ARRAY_SIZE,
	};
	static const string ModuleName;

public: //constructor / destructor
	CCallback(AMX *amx, int cb_idx) :
		m_AmxInstance(amx),
		m_AmxCallbackIndex(cb_idx)
	{ }
	~CCallback() = default;

private: //variables
	AMX *m_AmxInstance = nullptr;
	int m_AmxCallbackIndex = -1;
	cell m_AmxPushAddress = -1;

private: // functions
	// cell
	template<typename... Tv>
	void PushArgs(cell value, Tv&&... other_values)
	{
		PushArgs(std::forward<Tv>(other_values)...);
		amx_Push(m_AmxInstance, value);
	}

	// C string
	template<typename... Tv>
	void PushArgs(const char *value, Tv&&... other_values)
	{
		PushArgs(std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushString(m_AmxInstance, &tmp_addr, nullptr, value, 0, 0);

		if (m_AmxPushAddress < 0)
			m_AmxPushAddress = tmp_addr;
	}

	// std string
	template<typename... Tv>
	void PushArgs(std::string const &value, Tv&&... other_values)
	{
		PushArgs(std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushString(m_AmxInstance, &tmp_addr, nullptr, value.c_str(), 0, 0);

		if (m_AmxPushAddress < 0)
			m_AmxPushAddress = tmp_addr;
	}

	// cell array
	template<size_t N, typename... Tv>
	void PushArgs(const cell(&array)[N], Tv&&... other_values)
	{
		PushArgs(std::forward<Tv>(other_values)...);

		cell tmp_addr;
		amx_PushArray(m_AmxInstance, &tmp_addr, nullptr, array, N);

		if (m_AmxPushAddress < 0)
			m_AmxPushAddress = tmp_addr;
	}

	// reference
	template<size_t N, typename... Tv>
	void PushArgs(cell *value, Tv&&... other_values)
	{
		PushArgs(std::forward<Tv>(other_values)...);
		amx_PushAddress(m_AmxInstance, value);
	}

	void PushArgs()
	{ }

public: //functions
	bool Execute();

	template<typename... Tv>
	bool Execute(Tv&&... params)
	{
		PushArgs(std::forward<Tv>(params)...);
		return Execute();
	}

public: //factory functions
	static Callback_t Create(CError<CCallback> &error, AMX *amx, const char *name);

};


class CCallbackManager : public CSingleton<CCallbackManager>
{
	friend class CSingleton<CCallbackManager>;
private: //constructor / destructor
	CCallbackManager() = default;
	~CCallbackManager() = default;


private: //variables
	unordered_set<const AMX *> m_AmxInstances;

public: //functions
	inline bool IsValidAmx(const AMX *amx)
	{
		return m_AmxInstances.count(amx) == 1;
	}

	inline void AddAmx(const AMX *amx)
	{
		m_AmxInstances.insert(amx);
	}
	inline void RemoveAmx(const AMX *amx)
	{
		m_AmxInstances.erase(amx);
	}

};
