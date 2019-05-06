#include "sdk.hpp"
#include "natives.hpp"
#include "PawnDispatcher.hpp"
#include "Network.hpp"
#include "Guild.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "SampConfigReader.hpp"
#include "Logger.hpp"
#include "version.hpp"

#include <samplog/samplog.hpp>
#include <thread>
#include <cstdlib>


extern void	*pAMXFunctions;
logprintf_t logprintf;


void InitializeEverything(std::string const &bot_token)
{
	GuildManager::Get()->Initialize();
	UserManager::Get()->Initialize();
	ChannelManager::Get()->Initialize();
	MessageManager::Get()->Initialize();

	Network::Get()->Initialize(bot_token);
}

void DestroyEverything()
{
	MessageManager::Singleton::Destroy();
	ChannelManager::Singleton::Destroy();
	UserManager::Singleton::Destroy();
	GuildManager::Singleton::Destroy();
	Network::Singleton::Destroy();
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

std::string GetEnvironmentVar(const char *key)
{
	return std::string(getenv(key));
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	
	bool ret_val = true;
	auto bot_token = GetEnvironmentVar("SAMP_DISCORD_BOT_TOKEN");

	if(bot_token.empty())
		SampConfigReader::Get()->GetVar("discord_bot_token", bot_token);

	if (!bot_token.empty())
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
		logprintf(" >> plugin.dc-connector: bot token not specified in environment variable or server config.");
		ret_val = false;
	}
	return ret_val;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("plugin.dc-connector: Unloading plugin...");

	DestroyEverything();
	Logger::Singleton::Destroy();
	
	samplog::Api::Destroy();
	
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
	AMX_DEFINE_NATIVE(DCC_GetChannelPosition)
	AMX_DEFINE_NATIVE(DCC_IsChannelNsfw)
	AMX_DEFINE_NATIVE(DCC_SendChannelMessage)
	AMX_DEFINE_NATIVE(DCC_SetChannelName)
	AMX_DEFINE_NATIVE(DCC_SetChannelTopic)
	AMX_DEFINE_NATIVE(DCC_SetChannelPosition)
	AMX_DEFINE_NATIVE(DCC_SetChannelNsfw)
	AMX_DEFINE_NATIVE(DCC_SetChannelParentCategory)
	AMX_DEFINE_NATIVE(DCC_DeleteChannel)

	AMX_DEFINE_NATIVE(DCC_GetMessageId)
	AMX_DEFINE_NATIVE(DCC_GetMessageChannel)
	AMX_DEFINE_NATIVE(DCC_GetMessageAuthor)
	AMX_DEFINE_NATIVE(DCC_GetMessageContent)
	AMX_DEFINE_NATIVE(DCC_IsMessageTts)
	AMX_DEFINE_NATIVE(DCC_IsMessageMentioningEveryone)
	AMX_DEFINE_NATIVE(DCC_GetMessageUserMentionCount)
	AMX_DEFINE_NATIVE(DCC_GetMessageUserMention)
	AMX_DEFINE_NATIVE(DCC_GetMessageRoleMentionCount)
	AMX_DEFINE_NATIVE(DCC_GetMessageRoleMention)
	AMX_DEFINE_NATIVE(DCC_DeleteMessage)

	AMX_DEFINE_NATIVE(DCC_FindUserByName)
	AMX_DEFINE_NATIVE(DCC_FindUserById)
	AMX_DEFINE_NATIVE(DCC_GetUserId)
	AMX_DEFINE_NATIVE(DCC_GetUserName)
	AMX_DEFINE_NATIVE(DCC_GetUserDiscriminator)
	AMX_DEFINE_NATIVE(DCC_IsUserBot)
	AMX_DEFINE_NATIVE(DCC_IsUserVerified)

	AMX_DEFINE_NATIVE(DCC_FindRoleByName)
	AMX_DEFINE_NATIVE(DCC_FindRoleById)
	AMX_DEFINE_NATIVE(DCC_GetRoleId)
	AMX_DEFINE_NATIVE(DCC_GetRoleName)
	AMX_DEFINE_NATIVE(DCC_GetRoleColor)
	AMX_DEFINE_NATIVE(DCC_GetRolePermissions)
	AMX_DEFINE_NATIVE(DCC_IsRoleHoist)
	AMX_DEFINE_NATIVE(DCC_GetRolePosition)
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
	AMX_DEFINE_NATIVE(DCC_SetGuildName)
	AMX_DEFINE_NATIVE(DCC_CreateGuildChannel)
	AMX_DEFINE_NATIVE(DCC_GetCreatedGuildChannel)
	
	{ NULL, NULL }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	samplog::Api::Get()->RegisterAmx(amx);
	pawn_cb::AddAmx(amx);
	return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	samplog::Api::Get()->EraseAmx(amx);
	pawn_cb::RemoveAmx(amx);
	return AMX_ERR_NONE;
}
