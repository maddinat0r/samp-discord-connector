#include "Logger.hpp"

#include <amx/amx.h>


void DebugInfoManager::Update(AMX * const amx, const char *func)
{
	m_Amx = amx;
	m_NativeName = func;
	m_Info.clear();
	m_Available = samplog::Api::Get()->GetAmxFunctionCallTrace(amx, m_Info);
}

void DebugInfoManager::Clear()
{
	m_Amx = nullptr;
	m_NativeName = nullptr;
	m_Available = false;
}

ScopedDebugInfo::ScopedDebugInfo(AMX * const amx, const char *func,
	cell * const params, const char *params_format /* = ""*/)
{
	DebugInfoManager::Get()->Update(amx, func);

	auto &logger = Logger::Get()->m_Logger;
	if (logger.IsLogLevel(samplog_LogLevel::DEBUG))
		logger.LogNativeCall(amx, params, func, params_format);
}
