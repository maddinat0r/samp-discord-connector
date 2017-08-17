#include "sdk.hpp"
#include "natives.hpp"
#include "CDispatcher.hpp"
#include "CCallback.hpp"
#include "Network.hpp"
#include "CChannel.hpp"
#include "SampConfigReader.hpp"
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
	CLog::Get()->SetLogLevel(LogLevel::DEBUG | LogLevel::INFO | LogLevel::WARNING | LogLevel::ERROR);

	bool ret_val = true;
	std::string bot_token;
	if (SampConfigReader::Get()->GetVar("discord_bot_token", bot_token))
	{
		CChannelManager::Get()->Initialize();
		Network::Get()->Initialize(bot_token);
		
		CChannelManager::Get()->WaitForInitialization();

		logprintf(" >> plugin.dc-connector: " PLUGIN_VERSION " successfully loaded.");
	}
	else
	{
		logprintf(" >> plugin.dc-connector: bot token not specified in server config.");
		ret_val = false;
	}
	return ret_val;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("plugin.dc-connector: Unloading plugin...");
	
	CChannelManager::CSingleton::Destroy();
	Network::CSingleton::Destroy();
	PawnCallbackManager::CSingleton::Destroy();
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
	AMX_DEFINE_NATIVE(DCC_FindChannelByName)
	AMX_DEFINE_NATIVE(DCC_FindChannelById)
	AMX_DEFINE_NATIVE(DCC_IsChannelPrivate)
	AMX_DEFINE_NATIVE(DCC_GetChannelName)
	AMX_DEFINE_NATIVE(DCC_GetChannelId)
	AMX_DEFINE_NATIVE(DCC_GetChannelTopic)
	AMX_DEFINE_NATIVE(DCC_SendChannelMessage)
	
	{ NULL, NULL }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	samplog::RegisterAmx(amx);
	PawnCallbackManager::Get()->AddAmx(amx);
	return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	samplog::EraseAmx(amx);
	PawnCallbackManager::Get()->RemoveAmx(amx);
	return AMX_ERR_NONE;
}
