#include "sdk.hpp"
#include "natives.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"
#include "Network.hpp"
#include "Guild.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "SampConfigReader.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <samplog/DebugInfo.h>
#include <thread>


extern void	*pAMXFunctions;
logprintf_t logprintf;


void InitializeEverything(std::string const &bot_token)
{
	GuildManager::Get()->Initialize();
	UserManager::Get()->Initialize();
	ChannelManager::Get()->Initialize();

	Network::Get()->Initialize(bot_token);
}

void DestroyEverything()
{
	ChannelManager::CSingleton::Destroy();
	UserManager::CSingleton::Destroy();
	GuildManager::CSingleton::Destroy();
	Network::CSingleton::Destroy();
}

bool WaitForInitialization()
{
	unsigned int const
		SLEEP_TIME_MS = 20,
		TIMEOUT_TIME_MS = 20 * 1000;
	unsigned int waited_time = 0;
	while (true)
	{
		if (GuildManager::Get()->IsInitialized()
			&& UserManager::Get()->IsInitialized()
			&& ChannelManager::Get()->IsInitialized())
		{
			return true;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
		waited_time += SLEEP_TIME_MS;
		if (waited_time > TIMEOUT_TIME_MS)
			break;
	}
	return false;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	
	samplog::Init();
	CLog::Get()->SetLogLevel(LogLevel::INFO | LogLevel::WARNING | LogLevel::ERROR);

	bool ret_val = true;
	std::string bot_token;
	if (SampConfigReader::Get()->GetVar("discord_bot_token", bot_token))
	{
		InitializeEverything(bot_token);

		if (WaitForInitialization())
		{
			logprintf(" >> plugin.dc-connector: " PLUGIN_VERSION " successfully loaded.");
		}
		else
		{
			logprintf(" >> plugin.dc-connector: timeout while initializing data.");

			std::thread init_thread([bot_token]()
			{
				while (true)
				{
					std::this_thread::sleep_for(std::chrono::minutes(2));

					DestroyEverything();
					InitializeEverything(bot_token);
					if (WaitForInitialization())
						break;
				}
			});
			init_thread.detach();

			logprintf("                         plugin will proceed to retry connecting in the background.");
		}
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

	DestroyEverything();
	PawnCallbackManager::CSingleton::Destroy();
	CLog::CSingleton::Destroy();
	
	samplog::Exit();
	
	logprintf("plugin.dc-connector: Plugin unloaded.");
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	PawnDispatcher::Get()->Process();
}


extern "C" const AMX_NATIVE_INFO native_list[] =
{
	//AMX_DEFINE_NATIVE(native_name)
	AMX_DEFINE_NATIVE(DCC_FindChannelByName)
	AMX_DEFINE_NATIVE(DCC_FindChannelById)
	AMX_DEFINE_NATIVE(DCC_GetChannelId)
	AMX_DEFINE_NATIVE(DCC_GetChannelType)
	AMX_DEFINE_NATIVE(DCC_GetChannelGuild)
	AMX_DEFINE_NATIVE(DCC_GetChannelName)
	AMX_DEFINE_NATIVE(DCC_GetChannelTopic)
	AMX_DEFINE_NATIVE(DCC_SendChannelMessage)

	AMX_DEFINE_NATIVE(DCC_FindUserByName)
	AMX_DEFINE_NATIVE(DCC_FindUserById)
	AMX_DEFINE_NATIVE(DCC_GetUserId)
	AMX_DEFINE_NATIVE(DCC_GetUserName)
	AMX_DEFINE_NATIVE(DCC_GetUserDiscriminator)
	AMX_DEFINE_NATIVE(DCC_GetUserEmail)
	AMX_DEFINE_NATIVE(DCC_IsUserBot)
	AMX_DEFINE_NATIVE(DCC_IsUserVerified)

	AMX_DEFINE_NATIVE(DCC_FindRoleByName)
	AMX_DEFINE_NATIVE(DCC_FindRoleById)
	AMX_DEFINE_NATIVE(DCC_GetRoleId)
	AMX_DEFINE_NATIVE(DCC_GetRoleName)
	AMX_DEFINE_NATIVE(DCC_GetRoleColor)
	AMX_DEFINE_NATIVE(DCC_GetRolePermissions)
	AMX_DEFINE_NATIVE(DCC_IsRoleHoist)
	AMX_DEFINE_NATIVE(DCC_IsRoleMentionable)

	AMX_DEFINE_NATIVE(DCC_FindGuildByName)
	AMX_DEFINE_NATIVE(DCC_FindGuildById)
	AMX_DEFINE_NATIVE(DCC_GetGuildId)
	AMX_DEFINE_NATIVE(DCC_GetGuildName)
	AMX_DEFINE_NATIVE(DCC_GetGuildOwnerId)
	AMX_DEFINE_NATIVE(DCC_GetGuildRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildRoleCount)
	AMX_DEFINE_NATIVE(DCC_GetGuildMember)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberCount)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberNickname)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberRoleCount)
	AMX_DEFINE_NATIVE(DCC_HasGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberStatus)
	AMX_DEFINE_NATIVE(DCC_GetGuildChannel)
	AMX_DEFINE_NATIVE(DCC_GetGuildChannelCount)
	AMX_DEFINE_NATIVE(DCC_GetAllGuilds)
	
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
