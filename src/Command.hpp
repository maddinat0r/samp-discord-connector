#pragma once

#include "types.hpp"
#include "Singleton.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"

#include <string>
#include <vector>

#include <json.hpp>

using json = nlohmann::json;
static CommandOption_t InvalidCommandOption;
static Command_t InvalidCommand;

enum class COMMAND_OPTION_TYPE : int
{
	OPTION_SUB_COMMAND = 1,
	OPTION_SUB_COMMAND_GROUP = 2,
	OPTION_STRING = 3,
	OPTION_INTEGER = 4,
	OPTION_BOOLEAN = 5,
	OPTION_USER = 6,
	OPTION_CHANNEL = 7,
	OPTION_ROLE = 8
};

class CommandOption
{
private:
	Command_t & m_CommandParent = InvalidCommand;
	CommandOption_t & m_OptionParent = InvalidCommandOption;
	COMMAND_OPTION_TYPE m_Type;
	std::map<std::string, CommandOption_t> m_ChildOptions;
	std::string m_Name;
	std::string m_Description;
	bool m_Required = true;
public:
	CommandOption() {};

	void SetType(COMMAND_OPTION_TYPE type)
	{
		m_Type = type;
	}

	void AddOption(std::string const & name, CommandOption_t & option)
	{
		m_ChildOptions.insert(std::make_pair(name, std::move(option)));
	}

	void SetName(std::string name)
	{
		m_Name = name;
	}

	void SetDescription(std::string description)
	{
		m_Description = description;
	}

	void SetRequired(bool required)
	{
		m_Required = required;
	}

	COMMAND_OPTION_TYPE GetType()
	{
		return m_Type;
	}

	std::map<std::string, CommandOption_t> & GetChildOptions()
	{
		return m_ChildOptions;
	}

	std::string const & GetName()
	{
		return m_Name;
	}

	std::string const & GetDescription()
	{
		return m_Description;
	}

	bool IsRequired()
	{
		return m_Required;
	}

	CommandOption_t const & GetOptionParent()
	{
		return m_OptionParent;
	}
};


class Command
{
public:
	Command(Snowflake_t const& id, std::string const& name, std::string const& description, GuildId_t guild);
	~Command() = default;

	void AddOption(std::string const & name, CommandOption_t & option)
	{
		m_Options.insert(std::make_pair(name, std::move(option)));
	}

	std::string const& GetName()
	{
		return m_Name;
	}

	GuildId_t const & GetGuild()
	{
		return m_Guild;
	}

	std::string const& GetCallback()
	{
		return m_Callback;
	}

	nlohmann::json DumpToJson();
	void DumpOptionToJson(CommandOption_t & option, nlohmann::json& command_json);
	void HandleNewCreation();
	void SetCallback(std::string const& callback);
	void SetGuild(const GuildId_t guild) {
		m_Guild = guild;
	}

	void AllowEveryone(bool allow) {
		m_DefaultEveryone = allow;
	}
	/*void SetPermission(RoleId_t role, bool permission) {
		m_Permissions.emplace(role, permission);
	}*/
	void DeleteFromDiscord();
private:
	Snowflake_t m_ID;
	std::string m_Name;
	std::string m_Description;
	GuildId_t m_Guild;
	bool m_DefaultEveryone = true;
	std::map<std::string, CommandOption_t> m_Options;
	//std::map<RoleId_t, bool> m_Permissions;
	std::string m_Callback;
	bool m_Valid = false;
};

class CommandManager : public Singleton<CommandManager>
{
	friend class Singleton<CommandManager>;
private:
	CommandManager() = default;
	~CommandManager() = default;

private:
	std::map<CommandId_t, Command_t> m_Commands;
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };
	unsigned int m_InitGuilds = 0;
public:
	void Initialize();
	void ParseOptionData(Command_t& command, nlohmann::json& option_json, CommandOption_t & parent_option = InvalidCommandOption);
	void ParseCommandCreationData(nlohmann::json& command, GuildId_t guild = INVALID_GUILD_ID);
	Command_t & AddCommandInternal(Snowflake_t const& snowflake, std::string const& name, std::string const& description, GuildId_t guild);
	CommandId_t AddCommand(std::string const & name, std::string const & description);
	bool DeleteCommand(CommandId_t id);
	Command_t const & FindCommand(CommandId_t command);
	CommandId_t const & FindCommandIdByName(std::string const& name, GuildId_t guild = INVALID_GUILD_ID);

	bool IsInitialized()
	{
		return m_Initialized == m_InitValue;
	}
};	