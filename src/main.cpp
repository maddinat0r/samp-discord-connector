#include "sdk.hpp"
#include "natives.hpp"
#include "PawnDispatcher.hpp"
#include "Network.hpp"
#include "Guild.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "Command.hpp"
#include "SampConfigReader.hpp"
#include "Logger.hpp"
#include "version.hpp"

#include <samplog/samplog.hpp>
#include <thread>
#include <cstdlib>
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>

extern void	*pAMXFunctions;
logprintf_t logprintf;


void InitializeEverything(std::string const &bot_token)
{
	GuildManager::Get()->Initialize();
	UserManager::Get()->Initialize();
	ChannelManager::Get()->Initialize();
	MessageManager::Get()->Initialize();
	CommandManager::Get()->Initialize();
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
			&& ChannelManager::Get()->IsInitialized()
			&& CommandManager::Get()->IsInitialized())
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
	const char *value = getenv(key);
	return value != nullptr ? std::string(value) : std::string();
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

	bool ret_val = true;
	auto bot_token = GetEnvironmentVar("SAMP_DISCORD_BOT_TOKEN");

	if (bot_token.empty())
		SampConfigReader::Get()->GetVar("discord_bot_token", bot_token);

	if (!bot_token.empty())
	{
		InitializeEverything(bot_token);

		if (WaitForInitialization())
		{
			logprintf(" >> discord-connector: " PLUGIN_VERSION " successfully loaded.");
		}
		else
		{
			logprintf(" >> discord-connector: timeout while initializing data.");

			std::thread init_thread([bot_token]()
			{
				while (true)
				{
					std::this_thread::sleep_for(std::chrono::minutes(1));

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
		logprintf(" >> discord-connector: bot token not specified in environment variable or server config.");
		ret_val = false;
	}
	return ret_val;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("discord-connector: Unloading plugin...");

	DestroyEverything();
	Logger::Singleton::Destroy();

	samplog::Api::Destroy();

	logprintf("discord-connector: Plugin unloaded.");
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
	AMX_DEFINE_NATIVE(DCC_GetChannelParentCategory)
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
	AMX_DEFINE_NATIVE(DCC_GetCreatedMessage)

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
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberVoiceChannel)
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
	AMX_DEFINE_NATIVE(DCC_SetGuildMemberNickname)
	AMX_DEFINE_NATIVE(DCC_SetGuildMemberVoiceChannel)
	AMX_DEFINE_NATIVE(DCC_AddGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_RemoveGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_RemoveGuildMember)
	AMX_DEFINE_NATIVE(DCC_CreateGuildMemberBan)
	AMX_DEFINE_NATIVE(DCC_RemoveGuildMemberBan)
	AMX_DEFINE_NATIVE(DCC_SetGuildRolePosition)
	AMX_DEFINE_NATIVE(DCC_SetGuildRoleName)
	AMX_DEFINE_NATIVE(DCC_SetGuildRolePermissions)
	AMX_DEFINE_NATIVE(DCC_SetGuildRoleColor)
	AMX_DEFINE_NATIVE(DCC_SetGuildRoleHoist)
	AMX_DEFINE_NATIVE(DCC_SetGuildRoleMentionable)
	AMX_DEFINE_NATIVE(DCC_CreateGuildRole)
	AMX_DEFINE_NATIVE(DCC_GetCreatedGuildRole)
	AMX_DEFINE_NATIVE(DCC_DeleteGuildRole)

	AMX_DEFINE_NATIVE(DCC_GetBotPresenceStatus)
	AMX_DEFINE_NATIVE(DCC_TriggerBotTypingIndicator)
	AMX_DEFINE_NATIVE(DCC_SetBotNickname)
	AMX_DEFINE_NATIVE(DCC_CreatePrivateChannel)
	AMX_DEFINE_NATIVE(DCC_GetCreatedPrivateChannel)
	AMX_DEFINE_NATIVE(DCC_SetBotPresenceStatus)
	AMX_DEFINE_NATIVE(DCC_SetBotActivity)

	AMX_DEFINE_NATIVE(DCC_EscapeMarkdown)

	AMX_DEFINE_NATIVE(DCC_CreateEmbed)
	AMX_DEFINE_NATIVE(DCC_DeleteEmbed)
	AMX_DEFINE_NATIVE(DCC_SendChannelEmbedMessage)
	AMX_DEFINE_NATIVE(DCC_AddEmbedField)
	AMX_DEFINE_NATIVE(DCC_SetEmbedTitle)
	AMX_DEFINE_NATIVE(DCC_SetEmbedDescription)
	AMX_DEFINE_NATIVE(DCC_SetEmbedUrl)
	AMX_DEFINE_NATIVE(DCC_SetEmbedTimestamp)
	AMX_DEFINE_NATIVE(DCC_SetEmbedColor)
	AMX_DEFINE_NATIVE(DCC_SetEmbedFooter)
	AMX_DEFINE_NATIVE(DCC_SetEmbedThumbnail)
	AMX_DEFINE_NATIVE(DCC_SetEmbedImage)

	AMX_DEFINE_NATIVE(DCC_DeleteInternalMessage)

	AMX_DEFINE_NATIVE(DCC_CreateEmoji)
	AMX_DEFINE_NATIVE(DCC_DeleteEmoji)
	AMX_DEFINE_NATIVE(DCC_GetEmojiName)

	AMX_DEFINE_NATIVE(DCC_CreateReaction)
	AMX_DEFINE_NATIVE(DCC_DeleteMessageReaction)

	AMX_DEFINE_NATIVE(DCC_EditMessage)
	AMX_DEFINE_NATIVE(DCC_SetMessagePersistent)
	AMX_DEFINE_NATIVE(DCC_CacheChannelMessage)

	AMX_DEFINE_NATIVE(DCC_CreateCommand)
	AMX_DEFINE_NATIVE(DCC_GetInteractionMentionCount)
	AMX_DEFINE_NATIVE(DCC_GetInteractionMention)
	AMX_DEFINE_NATIVE(DCC_GetInteractionContent)
	AMX_DEFINE_NATIVE(DCC_GetInteractionChannel)
	AMX_DEFINE_NATIVE(DCC_GetInteractionGuild)
	AMX_DEFINE_NATIVE(DCC_SendInteractionEmbed)
	AMX_DEFINE_NATIVE(DCC_SendInteractionMessage)
	AMX_DEFINE_NATIVE(DCC_DeleteCommand)
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

class DiscordComponent;

DiscordComponent* discordComponent = nullptr;

class DiscordComponent : public IComponent, public PawnEventHandler, public CoreEventHandler
{
	PROVIDE_UID(0x493DFE4F6EA1841F);
	IPawnComponent* pawnComponent = nullptr;
	static ICore* core;
	
	static void logprintfwrapped(char const* format, ...)
	{
		va_list params;
		va_start(params, format);
		core->vlogLn(LogLevel::Message, format, params);
		va_end(params);
	}
#
	StringView componentName() const override
	{
		return "discord-connector";
	}

	SemanticVersion componentVersion() const override
	{
		return SemanticVersion(0, 0, 0, 0);
	}

	void onLoad(ICore* c) override
	{
		core = c;
		logprintf = DiscordComponent::logprintfwrapped;

		bool ret_val = true;
		auto bot_token = GetEnvironmentVar("SAMP_DISCORD_BOT_TOKEN");

		if (bot_token.empty()) {
			auto token = core->getConfig().getString("discord.bot_token");
			if (!token.empty()) {
				bot_token = token.data();
			}
		}
			

		if (!bot_token.empty())
		{
			InitializeEverything(bot_token.data());

			if (WaitForInitialization())
			{
				logprintf(" >> discord-connector: " PLUGIN_VERSION " successfully loaded.");
			}
			else
			{
				logprintf(" >> discord-connector: timeout while initializing data.");

				std::thread init_thread([bot_token]()
					{
						while (true)
						{
							std::this_thread::sleep_for(std::chrono::minutes(1));

							DestroyEverything();
							InitializeEverything(bot_token.data());
							if (WaitForInitialization())
								break;
						}
					});
				init_thread.detach();

				logprintf("                         component will proceed to retry connecting in the background.");
			}
		}
		else
		{
			logprintf(" >> discord-connector: bot token not specified in environment variable or server config.");
			ret_val = false;
		}
	}

	void onInit(IComponentList* components) override
	{
		pawnComponent = components->queryComponent<IPawnComponent>();
		core->getEventDispatcher().addEventHandler(this);
		if (pawnComponent)
		{
			pAMXFunctions = (void*)&pawnComponent->getAmxFunctions();
			pawnComponent->getEventDispatcher().addEventHandler(this, EventPriority_FairlyHigh);
		}
		else
		{
			core->logLn(LogLevel::Error, "discord-connector: Pawn component not loaded.");
		}
	}

	void onAmxLoad(IPawnScript& script) override
	{
		amx_Register(script.GetAMX(), native_list, -1);
		samplog::Api::Get()->RegisterAmx(script.GetAMX());
		pawn_cb::AddAmx(script.GetAMX());
	}

	void onAmxUnload(IPawnScript& script) override
	{
		samplog::Api::Get()->EraseAmx(script.GetAMX());
		pawn_cb::RemoveAmx(script.GetAMX());
	}

	void onFree(IComponent* component) override
	{
		if (component == pawnComponent)
		{
			pawnComponent = nullptr;
			pAMXFunctions = nullptr;
		}
	}

	void free() override
	{
		if (pawnComponent)
		{
			pawnComponent->getEventDispatcher().removeEventHandler(this);
		}
		core->getEventDispatcher().removeEventHandler(this);

		logprintf("discord-connector: Unloading componment...");

		DestroyEverything();
		Logger::Singleton::Destroy();

		samplog::Api::Destroy();

		logprintf("discord-connector: Component unloaded.");
		delete this;
	}

	void reset() override
	{
		// Nothing to reset for now.
	}

	void provideConfiguration(ILogger& logger, IEarlyConfig& config, bool defaults) override
	{
		if (defaults)
		{
			config.setString("discord.bot_token", "");
		}
		else
		{
			if (config.getType("discord.bot_token") == ConfigOptionType_None)
			{
				config.setString("discord.bot_token", "");
			}
		}
	}

	void onTick(Microseconds elapsed, TimePoint now) override
	{
		PawnDispatcher::Get()->Process();
	}
};

ICore* DiscordComponent::core = nullptr;

COMPONENT_ENTRY_POINT()
{
	discordComponent = new DiscordComponent();
	return discordComponent;
}