#include "CCallback.hpp"
#include "CLog.hpp"


const string CCallback::ModuleName{ "callback" };


Callback_t CCallback::Create(CError<CCallback> &error, AMX *amx, const char *name)
{
	CLog::Get()->Log(LogLevel::DEBUG,
					 "CCallback::Create(amx={}, name='{}')",
					 static_cast<const void *>(amx),
					 name ? name : "(nullptr)");

	if (amx == nullptr)
	{
		error.set(Error::INVALID_AMX, "invalid AMX");
		return nullptr;
	}

	if (name == nullptr || strlen(name) == 0)
	{
		error.set(Error::EMPTY_NAME, "empty name specified");
		return nullptr;
	}

	int cb_idx = -1;
	if (amx_FindPublic(amx, name, &cb_idx) != AMX_ERR_NONE)
	{
		error.set(Error::NOT_FOUND, "callback \"{}\" does not exist", name);
		return nullptr;
	}

	CLog::Get()->Log(LogLevel::DEBUG, 
					 "CCallback::Create - callback index for '{}': {}",
					 name, cb_idx);

	CLog::Get()->Log(LogLevel::INFO, 
					 "Callback '{}' set up for delayed execution.", name);
	return std::make_shared<CCallback>(amx, cb_idx);
}


bool CCallback::Execute()
{
	CLog::Get()->Log(LogLevel::DEBUG, 
					 "CCallback::Execute(amx={}, index={})",
					 static_cast<const void *>(m_AmxInstance),
					 m_AmxCallbackIndex);

	//the user could unload a filterscript between CCallback creation and
	//execution, so we better check if the AMX instance is still valid
	if (CCallbackManager::Get()->IsValidAmx(m_AmxInstance) == false)
	{
		CLog::Get()->Log(LogLevel::ERROR, 
						 "CCallback::Execute - invalid AMX instance");
		return false;
	}

	if (CLog::Get()->IsLogLevel(LogLevel::INFO))
	{
		char callback_name[sNAMEMAX + 1];
		if (amx_GetPublic(m_AmxInstance, m_AmxCallbackIndex, callback_name) == AMX_ERR_NONE)
		{
			CLog::Get()->Log(LogLevel::INFO, "Executing callback '{}'...", callback_name);
		}
	}

	CLog::Get()->Log(LogLevel::DEBUG, 
					 "executing AMX callback with index '{}'", 
					 m_AmxCallbackIndex);

	int error = amx_Exec(m_AmxInstance, nullptr, m_AmxCallbackIndex);

	CLog::Get()->Log(LogLevel::DEBUG, 
					 "AMX callback executed with error '{}'", 
					 error);

	if (m_AmxPushAddress >= 0)
		amx_Release(m_AmxInstance, m_AmxPushAddress);
	m_AmxPushAddress = -1;

	CLog::Get()->Log(LogLevel::INFO, "Callback successfully executed.");

	return true;
}
