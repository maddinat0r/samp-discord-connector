#include "sdk.hpp"
#include "natives.hpp"
#include "CDispatcher.hpp"
#include "CCallback.hpp"
#include "CNetwork.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <samplog/DebugInfo.h>


extern void	*pAMXFunctions;
logprintf_t logprintf;


PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	
	samplog::Init();
	CLog::Get()->SetLogLevel(static_cast<LogLevel>(LogLevel::DEBUG | LogLevel::INFO | LogLevel::WARNING | LogLevel::ERROR));
	
	logprintf(" >> plugin.dc-connector: " PLUGIN_VERSION " successfully loaded.");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("plugin.dc-connector: Unloading plugin...");
	
	CNetwork::CSingleton::Destroy();
	CCallbackManager::CSingleton::Destroy();
	CLog::CSingleton::Destroy();
	
	samplog::Exit();
	
	logprintf("plugin.dc-connector: Plugin unloaded.");
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	CDispatcher::Get()->Process();
}


extern "C" const AMX_NATIVE_INFO native_list[] =
{
	//AMX_DEFINE_NATIVE(native_name)
	AMX_DEFINE_NATIVE(DCC_Connect)
	AMX_DEFINE_NATIVE(DCC_FindChannelByName)
	AMX_DEFINE_NATIVE(DCC_FindChannelById)
	AMX_DEFINE_NATIVE(DCC_SendChannelMessage)
	
	{ NULL, NULL }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	samplog::RegisterAmx(amx);
	CCallbackManager::Get()->AddAmx(amx);
	return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	samplog::EraseAmx(amx);
	CCallbackManager::Get()->RemoveAmx(amx);
	return AMX_ERR_NONE;
}
