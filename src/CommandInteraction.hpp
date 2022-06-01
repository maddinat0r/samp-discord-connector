#pragma once

#include "types.hpp"
#include "Singleton.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Command.hpp"

#include <string>
#include <vector>

#include <json.hpp>
#include <boost/variant.hpp>

using json = nlohmann::json;
static CommandInteractionOption_t InvalidCommandInteractionChildOption;

struct CommandInteractionOption
{
	COMMAND_OPTION_TYPE m_Type;
	std::string m_Name;
	std::string m_Value;
};

class CommandInteraction
{
public:
	CommandInteraction(CommandInteractionId_t id, UserId_t user, nlohmann::json const& interaction_json);
	~CommandInteraction() = default;
	void ParseOptions(nlohmann::json const& interaction_json, std::string const& guildid);
	void AddInteractionOption(CommandInteractionOption_t &option);

	CommandInteractionId_t GetPawnId()
	{
		return m_ID;
	}

	GuildId_t GetGuildID()
	{
		return m_Guild;
	}

	ChannelId_t GetChannelID()
	{
		return m_Channel;
	}

	std::vector<UserId_t> const& GetMentions()
	{
		return m_Mentions;
	}

	std::vector<CommandInteractionOption_t> const& GetOptions()
	{
		return m_InteractionOptions;
	}

	void SendEmbed(EmbedId_t embedid, const std::string message = "");
	void SendInteractionMessage(const std::string message);

private:
	CommandInteractionId_t m_ID;
	Snowflake_t m_IDSnowflake;
	Snowflake_t m_Token;
	ChannelId_t m_Channel;
	GuildId_t m_Guild = 0;
	UserId_t m_InteractionUser;
	std::vector<CommandInteractionOption_t> m_InteractionOptions;
	std::vector<UserId_t> m_Mentions;
};

class CommandInteractionManager : public Singleton<CommandInteractionManager>
{
	friend class Singleton<CommandInteractionManager>;
private:
	CommandInteractionManager() = default;
	~CommandInteractionManager() = default;

private:
	std::map<CommandInteractionId_t, CommandInteraction_t> m_Interactions;
public:
	CommandInteraction_t const& FindCommandInteraction(CommandInteractionId_t interaction);
	CommandInteractionId_t AddCommandInteraction(UserId_t user, nlohmann::json const& interaction_json);
	bool DeleteCommandInteraction(CommandInteractionId_t interaction);

	CommandInteractionId_t m_CurrentInteractionID = 0;
};