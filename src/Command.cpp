#include "Command.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include "Bot.hpp"
#include "Guild.hpp"
#include "CommandInteraction.hpp"
#include "User.hpp"
#include "Role.hpp"

Command::Command(Snowflake_t const& id, std::string const& name, std::string const& description, GuildId_t guild) :
	m_ID(id),
	m_Name(name),
	m_Description(description),
	m_Guild(guild),
	m_Valid(true)
{
}

nlohmann::json Command::DumpToJson()
{
	nlohmann::json dump_json;

	dump_json["name"] = m_Name;
	dump_json["description"] = m_Description;

	if (GetGuild() != INVALID_GUILD_ID && !m_DefaultEveryone) {
		dump_json["default_member_permissions"] = "0";
	}

	if (m_Options.size())
	{
		dump_json["options"] = json::array();
		for (auto& option : m_Options)
		{
			DumpOptionToJson(option.second, dump_json);
		}
	}
	return dump_json;
}

void Command::DumpOptionToJson(CommandOption_t& option, nlohmann::json& command_json)
{
	json option_json = json::object();
	option_json["type"] = option->GetType();
	option_json["name"] = option->GetName();
	option_json["description"] = option->GetDescription();
	option_json["required"] = option->IsRequired();
	
	if (option->GetChildOptions().size())
	{
		option_json["options"] = json::array();
		for (auto & options : option->GetChildOptions())
		{
			DumpOptionToJson(options.second, option_json);
		}
	}

	command_json.at("options") += option_json;
}

void Command::HandleNewCreation()
{
	std::string api;
	auto data = DumpToJson();
	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return;
	}

	if (GetGuild() != INVALID_GUILD_ID)
	{
		api = fmt::format("/applications/{:s}/guilds/{:s}/commands", ThisBot::Get()->GetApplicationID(), GuildManager::Get()->FindGuild(GetGuild())->GetId());
	}
	else
	{
		api = fmt::format("/applications/{:s}/commands", ThisBot::Get()->GetApplicationID());
	}

	Network::Get()->Http().Post(api, json_str, [this](Http::Response r)
	{
		if (r.status == 201)
		{
			Logger::Get()->Log(samplog_LogLevel::INFO, "successfully got a created response from discord for command '{:s}'", m_Name);
			json response = json::parse(r.body);
			m_ID = response.at("id").get<std::string>();

			/*if (m_Permissions.size() && GetGuild() != INVALID_GUILD_ID) {
				json permission;
				std::string api = fmt::format("/applications/{:s}/guilds/{:s}/commands/{:s}/permissions", ThisBot::Get()->GetApplicationID(),
					GuildManager::Get()->FindGuild(GetGuild())->GetId(), m_ID);

				for (auto permissions : m_Permissions) {
					
					const Role_t& role = RoleManager::Get()->FindRole(permissions.first);
					if (!role->IsValid()) {
						continue;
					}

					json field_array = json::object(
					{ 
						{"id", role->GetId()},
						{"type", 1 },
						{"permission", permissions.second}
					});
					permission["permissions"] += field_array;
				}

				std::string json_str;
				if (!utils::TryDumpJson(permission, json_str))
				{
					Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);
					return;
				}
				Logger::Get()->Log(samplog_LogLevel::ERROR, "{}", json_str);
				Network::Get()->Http().Put(api, json_str);
			}*/
		}
	});
}

void Command::SetCallback(std::string const& callback)
{
	m_Callback = callback;
}

void Command::DeleteFromDiscord()
{
	std::string delete_url = "";
	if (GetGuild() != INVALID_GUILD_ID) {
		auto const & guild = GuildManager::Get()->FindGuild(GetGuild());
		if (guild) {
			delete_url = fmt::format("/applications/{}/guilds/{}/commands/{}", ThisBot::Get()->GetApplicationID(), guild->GetId(), m_ID);
		}
	}
	else {
		delete_url = fmt::format("/applications/{}/commands/{}", ThisBot::Get()->GetApplicationID(), m_ID);
	}
	Network::Get()->Http().Delete(delete_url);
}

// Manager
void CommandManager::Initialize()
{
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json const& data)
	{
		(void)data;
		Network::Get()->Http().Get("/api/oauth2/applications/@me", [this](Http::Response r)
		{
			json response = json::parse(r.body);
			ThisBot::Get()->SetApplicationID(response.at("id"));

			Network::Get()->Http().Get(fmt::format("/applications/{:s}/commands", ThisBot::Get()->GetApplicationID()), [this](Http::Response commandr)
			{
					if (commandr.status == 200)
					{
						m_InitGuilds = GuildManager::Get()->GetGuilds().size();
						for (auto & guild : GuildManager::Get()->GetGuilds())
						{
							Network::Get()->Http().Get(fmt::format("/applications/{:s}/guilds/{:s}/commands", ThisBot::Get()->GetApplicationID(), GuildManager::Get()->FindGuild(guild)->GetId()),
								[this, guild, commandr](Http::Response guild_command)
							{
									if (guild_command.status == 200)
									{
										json commands = json::parse(guild_command.body);
										for (auto & command : commands.items())
										{
											ParseCommandCreationData(command.value(), guild);
										}
									}
									m_InitGuilds--;

									if (!m_InitGuilds) {
										json commands = json::parse(commandr.body);
										for (auto& command : commands.items())
										{
											ParseCommandCreationData(command.value());
										}
										m_Initialized++;
									}
							});
							if (!m_InitGuilds) {
								json commands = json::parse(commandr.body);
								for (auto& command : commands.items())
								{
									ParseCommandCreationData(command.value());
								}
								m_Initialized++;
							}
						}

						
					}
			});
		}, false);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::INTERACTION_CREATE, [this](const json& data)
	{
		if (data.find("type") != data.end() && data.at("type").get<int>() == 2 /*application command*/)
		{
			json my_awesome_json;
			std::string json_str;


			my_awesome_json["type"] = 5; // DeferredChannelMessageWithSource
			if (!utils::TryDumpJson(my_awesome_json, json_str))
			{
				Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);
				return;
			}

			Network::Get()->Http().Post(fmt::format("/interactions/{:s}/{:s}/callback", data.at("id").get<std::string>(), data.at("token").get<std::string>()), json_str);

			UserId_t userid = 0;

			std::string meh;
			bool has_guild = utils::TryGetJsonValue(data, meh, "guild_id");
			if (has_guild) {
				userid = UserManager::Get()->AddUser(data["member"]["user"]);
			}
			else {
				userid = UserManager::Get()->AddUser(data["user"]);
			}
			
			auto interactionid = CommandInteractionManager::Get()->AddCommandInteraction(userid, data);
			PawnDispatcher::Get()->Dispatch([userid, interactionid, data]() mutable
			{
				auto & interaction = CommandInteractionManager::Get()->FindCommandInteraction(interactionid);
				auto & command = CommandManager::Get()->FindCommand(CommandManager::Get()->FindCommandIdByName(data.at("data").at("name").get<std::string>(), interaction->GetGuildID()));
				if (interaction && command && userid)
				{
					// forward CallbackName(DCC_Interaction:interaction, DCC_User:user);	
					CommandInteractionManager::Get()->m_CurrentInteractionID = interaction->GetPawnId();
					pawn_cb::Error error;
					pawn_cb::Callback::CallFirst(error, command->GetCallback().c_str(), interaction->GetPawnId(), userid);	
					CommandInteractionManager::Get()->m_CurrentInteractionID = INVALID_COMMAND_INTERACTION_ID;
				}
				CommandInteractionManager::Get()->DeleteCommandInteraction(interaction->GetPawnId());
			});
		}
	});
}

void CommandManager::ParseOptionData(Command_t & command, nlohmann::json & option_json, CommandOption_t & parent_option)
{
	CommandOption tmpoption;
	tmpoption.SetType(static_cast<COMMAND_OPTION_TYPE>(option_json.at("type").get<int>()));
	tmpoption.SetName(option_json.at("name"));
	tmpoption.SetDescription(option_json.at("description"));

	if (option_json.find("required") != option_json.end())
	{
		tmpoption.SetRequired(option_json.at("required").get<bool>());
	}
	
	CommandOption_t option = std::unique_ptr<CommandOption>(new CommandOption(std::move(tmpoption)));
	if (option_json.find("options") != option_json.end() && option_json.at("options").size())
	{
		for (const auto & option_ : option_json.at("options").items())
		{
			ParseOptionData(command, option_.value(), option);
		}
	}

	if (parent_option)
	{
		parent_option->AddOption(option->GetName(), option);
	}
	else
	{
		command->AddOption(option->GetName(), option);
	}
}

void CommandManager::ParseCommandCreationData(nlohmann::json& command_json, GuildId_t guild)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "creating command from discord {} for guild {}", command_json.at("name"), guild);
	
	Command_t & command = AddCommandInternal(command_json.at("id"), command_json.at("name"), command_json.at("description"), guild);
	if (command_json.find("options") != command_json.end())
	{
		for (const auto & option : command_json.at("options").items())
		{
			ParseOptionData(command, option.value(), InvalidCommandOption);
		}
	}
}

Command_t & CommandManager::AddCommandInternal(Snowflake_t const& snowflake, std::string const& name, std::string const& description, GuildId_t guild)
{
	CommandId_t id = 1;
	while (m_Commands.find(id) != m_Commands.end())
		++id;
	static Command_t invalid_command;
	if (!m_Commands.emplace(id, Command_t(new Command(snowflake, name, description, guild))).first->second)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't create command: duplicate key '{}'", id);
		return invalid_command;
	}

	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully created command with id '{}'", id);
	return m_Commands.find(id)->second;
}

CommandId_t CommandManager::AddCommand(std::string const& name, std::string const & description)
{
	CommandId_t id = 1;
	while (m_Commands.find(id) != m_Commands.end())
		++id;

	if (!m_Commands.emplace(id, Command_t(new Command("", name, description, INVALID_GUILD_ID))).first->second)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't create command: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully created command with id '{}'", id);
	return id;
}

bool CommandManager::DeleteCommand(CommandId_t id)
{
	if (m_Commands.find(id) == m_Commands.end())
	{
		Logger::Get()->Log(samplog_LogLevel::WARNING, "attempted to delete command with id '{}' but it does not exist", id);
		return false;
	}

	m_Commands.erase(id);
	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully deleted command with id '{}'", id);
	return true;
}

Command_t const & CommandManager::FindCommand(CommandId_t id)
{
	static Command_t invalid_command;
	auto it = m_Commands.find(id);
	if (it == m_Commands.end())
		return invalid_command;
	return it->second;
}

CommandId_t const & CommandManager::FindCommandIdByName(std::string const& name, GuildId_t guild)
{
	for (const auto& command : m_Commands)
	{
		if (name == command.second->GetName() && guild == command.second->GetGuild())
		{
			return command.first;
		}
	}
	return INVALID_COMMAND_ID;
}