#include "CCallback.hpp"
#include "CLog.hpp"


void PawnCallbackManager::Execute(const char *name, AMX *amx, int cb_idx, cell args_push_addr)
{
	CLog::Get()->Log(LogLevel::DEBUG,
		"PawnCallbackManager::Execute(name={},amx={}, cb_idx={}, args_push_addr={})",
		name, static_cast<const void *>(amx), cb_idx, args_push_addr);

	CLog::Get()->Log(LogLevel::INFO, "executing PAWN callback '{}'...", name);

	int error = amx_Exec(amx, nullptr, cb_idx);
	if (error != AMX_ERR_NONE)
	{
		CLog::Get()->Log(LogLevel::ERROR, "error while executing PAWN callback '{}': {}",
			name, error);
	}

	if (args_push_addr >= 0)
		amx_Release(amx, args_push_addr);

	if (error == AMX_ERR_NONE)
	{
		CLog::Get()->Log(LogLevel::INFO, "PAWN callback successfully executed.");
	}
}
