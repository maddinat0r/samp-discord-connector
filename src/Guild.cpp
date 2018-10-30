#include "Guild.hpp"
#include "Network.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include "Role.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"
#include "CLog.hpp"
#include "utils.hpp"

#include <unordered_map>


Guild::Guild(GuildId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	if (!utils::TryGetJsonValue(data, m_Id, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Update(data);

	if (utils::IsValidJson(data, "channels", json::value_t::array))
	{
		for (auto &c : data["channels"])
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(c, pawn_id);
			if (channel_id == INVALID_CHANNEL_ID)
				continue;

			AddChannel(channel_id);
		}
	}

	if (utils::IsValidJson(data, "members", json::value_t::array))
	{
		for (auto &m : data["members"])
		{
			if (!utils::IsValidJson(m, "user", json::value_t::object))
			{
				// we break here because all other array entries are likely
				// to be invalid too, and we don't want to spam an error message
				// for every single element in this array
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"user\" in \"{}\"", m.dump());
				break;
			}

			Member member;
			member.UserId = UserManager::Get()->AddUser(m["user"]);
			member.Update(m);
			AddMember(std::move(member));
		}

		unsigned int member_count;
		if (!utils::TryGetJsonValue(data, member_count, "member_count")
			|| member_count != m_Members.size())
		{
			Network::Get()->WebSocket().RequestGuildMembers(m_Id);
		}
	}

	if (utils::IsValidJson(data, "presences", json::value_t::array))
	{
		for (auto &p : data["presences"])
		{
			Snowflake_t userid;
			if (!utils::TryGetJsonValue(p, userid, "user", "id"))
			{
				// see above on why we break here
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"user.id\" in \"{}\"", p.dump());
				break;
			}

			std::string status;
			if (!utils::TryGetJsonValue(p, status, "status"))
			{
				// see above on why we break here
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"status\" in \"{}\"", p.dump());
				break;
			}

			for (auto &m : m_Members)
			{
				User_t const &user = UserManager::Get()->FindUser(m.UserId);
				assert(user);
				if (user->GetId() == userid)
				{
					m.UpdatePresence(status);
					break;
				}
			}
		}
	}
}

void Guild::Member::Update(json &data)
{
	// we don't care about the user object, there's an extra event for users
	if (utils::IsValidJson(data, "roles", json::value_t::array))
	{
		Roles.clear();
		for (auto &mr : data["roles"])
		{
			if (!mr.is_string())
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: not a string: \"{}\"", mr.dump());
				break;
			}

			Snowflake_t sfid = mr.get<std::string>();
			Role_t const &role = RoleManager::Get()->FindRoleById(sfid);
			if (role)
			{
				Roles.push_back(role->GetPawnId());
			}
			else
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update member role: role id \"{}\" not cached", sfid);
			}
		}
	}

	if (data.find("nick") != data.end())
	{
		auto &nick_json = data["nick"];
		if (nick_json.is_string())
			Nickname = data["nick"].get<std::string>();
		else if (nick_json.is_null())
			Nickname.clear();
		else
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: invalid datatype for \"nick\" in \"{}\"", data.dump());
	}
}

void Guild::Member::UpdatePresence(std::string const &status)
{
	// "idle", "dnd", "online", or "offline"
	static const std::unordered_map<std::string, Member::PresenceStatus> status_map{
		{ "idle", Member::PresenceStatus::IDLE },
		{ "dnd", Member::PresenceStatus::DO_NOT_DISTURB },
		{ "online", Member::PresenceStatus::ONLINE },
		{ "offline", Member::PresenceStatus::OFFLINE }
	};

	Status = status_map.at(status);
}

void Guild::UpdateMember(UserId_t userid, json &data)
{
	for (auto &m : m_Members)
	{
		if (m.UserId != userid)
			continue;

		m.Update(data);
		break;
	}
}

void Guild::UpdateMemberPresence(UserId_t userid, std::string const &status)
{
	for (auto &m : m_Members)
	{
		if (m.UserId != userid)
			continue;

		m.UpdatePresence(status);
		break;
	}
}

void Guild::Update(json &data)
{
	utils::TryGetJsonValue(data, m_Name, "name");

	utils::TryGetJsonValue(data, m_OwnerId, "owner_id");

	if (utils::IsValidJson(data, "roles", json::value_t::array))
	{
		for (auto &r : data["roles"])
		{
			std::string role_id;
			if (!utils::TryGetJsonValue(r, role_id, "id"))
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"id\" in \"{}\"", r.dump());
				break;
			}

			auto const &role = RoleManager::Get()->FindRoleById(role_id);
			if (role)
				role->Update(r);
			else
				m_Roles.push_back(RoleManager::Get()->AddRole(r));
		}
	}
}


void GuildManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		if (!utils::IsValidJson(data, "guilds", json::value_t::array))
		{
			CLog::Get()->Log(LogLevel::FATAL,
				"invalid JSON: expected \"guilds\" in \"{}\"", data.dump());
			return;
		}

		m_InitValue += data["guilds"].size();
		m_Initialized++;
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_CREATE, [this](json &data)
	{
		if (!m_IsInitialized)
		{
			AddGuild(data);
			m_Initialized++;
		}
		else
		{
			PawnDispatcher::Get()->Dispatch([data]() mutable
			{
				auto const guild_id = GuildManager::Get()->AddGuild(data);
				if (guild_id == INVALID_GUILD_ID)
					return;

				// forward DCC_OnGuildCreate(DCC_Guild:guild);
				PawnCallbackManager::Get()->Call("DCC_OnGuildCreate", guild_id);
			});
		}
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_DELETE, [](json &data)
	{
		Snowflake_t sfid;
		if (!utils::TryGetJsonValue(data, sfid, "id"))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([sfid]() mutable
		{
			Guild_t const &guild = GuildManager::Get()->FindGuildById(sfid);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::WARNING,
					"can't delete guild: guild id \"{}\" not cached", sfid);
				return;
			}

			// forward DCC_OnGuildDelete(DCC_Guild:guild);
			PawnCallbackManager::Get()->Call("DCC_OnGuildDelete", guild->GetPawnId());

			GuildManager::Get()->DeleteGuild(guild);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_UPDATE, [](json &data)
	{
		Snowflake_t sfid;
		if (!utils::TryGetJsonValue(data, sfid, "id"))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data, sfid]() mutable
		{
			Guild_t const &guild = GuildManager::Get()->FindGuildById(sfid);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild: guild id \"{}\" not cached", sfid);
				return;
			}

			guild->Update(data);

			// forward DCC_OnGuildUpdate(DCC_Guild:guild);
			PawnCallbackManager::Get()->Call("DCC_OnGuildUpdate", guild->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_ADD, [](json &data)
	{
		if (!utils::IsValidJson(data, 
			"guild_id", json::value_t::string,
			"user", json::value_t::object,
			"roles", json::value_t::array))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\", \"user\" and \"roles\" " \
				"in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t sfid = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(sfid);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't add guild member: guild id \"{}\" not cached", sfid);
				return;
			}

			Guild::Member member;
			// returns correct user if he already exists
			auto const userid = UserManager::Get()->AddUser(data["user"]);
			member.UserId = userid;
			member.Update(data);

			guild->AddMember(std::move(member));

			// forward DCC_OnGuildMemberAdd(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberAdd", guild->GetPawnId(), userid);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_REMOVE, [](json &data)
	{
		if (!utils::IsValidJson(data, 
			"guild_id", json::value_t::string,
			"user", "id", json::value_t::string))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" and \"user.id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t guild_id = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't remove guild member: guild id \"{}\" not cached", guild_id);
				return;
			}

			Snowflake_t user_id = data["user"]["id"].get<std::string>();
			auto const &user = UserManager::Get()->FindUserById(user_id);
			if (!user)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't remove guild member: user id \"{}\" not cached", user_id);
				return;
			}

			// forward DCC_OnGuildMemberRemove(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberRemove", guild->GetPawnId(), user->GetPawnId());

			guild->RemoveMember(user->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_UPDATE, [](json &data)
	{
		if (!utils::IsValidJson(data,
			"guild_id", json::value_t::string,
			"user", "id", json::value_t::string))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" and \"user.id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t guild_id = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild member: guild id \"{}\" not cached", guild_id);
				return;
			}

			Snowflake_t user_id = data["user"]["id"].get<std::string>();
			auto const &user = UserManager::Get()->FindUserById(user_id);
			if (!user)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild member: user id \"{}\" not cached", user_id);
				return;
			}

			guild->UpdateMember(user->GetPawnId(), data);

			// forward DCC_OnGuildMemberUpdate(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberUpdate", guild->GetPawnId(), user->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_CREATE, [](json &data)
	{
		if (!utils::IsValidJson(data,
			"guild_id", json::value_t::string,
			"role", json::value_t::object))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" and \"role\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t guild_id = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't add guild role: guild id \"{}\" not cached", guild_id);
				return;
			}

			auto const role = RoleManager::Get()->AddRole(data["role"]);
			guild->AddRole(role);

			// forward DCC_OnGuildRoleCreate(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleCreate", guild->GetPawnId(), role);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_DELETE, [](json &data)
	{
		if (!utils::IsValidJson(data,
			"guild_id", json::value_t::string,
			"role_id", json::value_t::string))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" and \"role_id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t guild_id = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't delete guild role: guild id \"{}\" not cached", guild_id);
				return;
			}

			Snowflake_t role_id = data["role_id"].get<std::string>();
			auto const &role = RoleManager::Get()->FindRoleById(role_id);
			if (!role)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't delete guild role: role id \"{}\" not cached", role_id);
				return;
			}

			// forward DCC_OnGuildRoleDelete(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleDelete", guild->GetPawnId(), role->GetPawnId());

			guild->RemoveRole(role->GetPawnId());
			RoleManager::Get()->RemoveRole(role);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_UPDATE, [](json &data)
	{
		if (!utils::IsValidJson(data,
			"guild_id", json::value_t::string,
			"role", "id", json::value_t::string))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" and \"role.id\" in \"{}\"", data.dump());
			return;
		}

		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t guild_id = data["guild_id"].get<std::string>();
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild role: guild id \"{}\" not cached", guild_id);
				return;
			}

			Snowflake_t role_id = data["role"]["id"].get<std::string>();
			auto const &role = RoleManager::Get()->FindRoleById(role_id);
			if (!role)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild role: role id \"{}\" not cached", role_id);
				return;
			}

			role->Update(data["role"]);

			// forward DCC_OnGuildRoleUpdate(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleUpdate", guild->GetPawnId(), role->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::PRESENCE_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			std::string guild_id;
			if (!utils::TryGetJsonValue(data, guild_id, "guild_id"))
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"guild_id\" in \"{}\"", data.dump());
				return;
			}

			std::string user_id;
			if (!utils::TryGetJsonValue(data, user_id, "user", "id"))
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"user.id\" in \"{}\"", data.dump());
				return;
			}

			std::string status;
			if (!utils::TryGetJsonValue(data, status, "status"))
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"status\" in \"{}\"", data.dump());
				return;
			}

			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild member presence: guild id \"{}\" not cached", guild_id);
				return;
			}

			auto const &user = UserManager::Get()->FindUserById(user_id);
			if (!user)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update guild member presence: user id \"{}\" not cached", user_id);
				return;
			}

			guild->UpdateMemberPresence(user->GetPawnId(), status);

			// forward DCC_OnGuildMemberUpdate(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberUpdate", guild->GetPawnId(), user->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBERS_CHUNK, [](json &data)
	{
		Snowflake_t guild_id;
		if (!utils::TryGetJsonValue(data, guild_id, "guild_id"))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" in \"{}\"", data.dump());
			return;
		}

		if (!utils::IsValidJson(data, "members", json::value_t::array))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected array \"members\" in \"{}\"", data.dump());
		}

		PawnDispatcher::Get()->Dispatch([guild_id, data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(guild_id);
			if (!guild)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't sync offline guild members: guild id \"{}\" not cached", guild_id);
				return;
			}

			for (auto &m : data["members"])
			{
				if (!utils::IsValidJson(m, "user", json::value_t::object))
				{
					// we break here because all other array entries are likely
					// to be invalid too, and we don't want to spam an error message
					// for every single element in this array
					CLog::Get()->Log(LogLevel::ERROR,
						"invalid JSON: expected \"user\" in \"{}\"", m.dump());
					break;
				}

				Guild::Member member;
				// returns correct user if he already exists
				auto const userid = UserManager::Get()->AddUser(m["user"]);
				member.UserId = userid;
				member.Update(m);

				guild->AddMember(std::move(member));
			}
		});
	});
	// TODO: events
}

bool GuildManager::IsInitialized()
{
	if (!m_IsInitialized && m_Initialized == m_InitValue)
		m_IsInitialized = true;

	return m_IsInitialized;
}

GuildId_t GuildManager::AddGuild(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_GUILD_ID;
	}

	auto const &guild = FindGuildById(sfid);
	if (guild)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't add guild: guild id \"{}\" already exists (PAWN id '{}')", 
			sfid, guild->GetPawnId());
		return INVALID_GUILD_ID;
	}

	GuildId_t id = 1;
	while (m_Guilds.find(id) != m_Guilds.end())
		++id;

	if (!m_Guilds.emplace(id, Guild_t(new Guild(id, data))).second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create guild: duplicate key '{}'", id);
		return INVALID_GUILD_ID;
	}

	CLog::Get()->Log(LogLevel::INFO, "successfully created guild with id '{}'", id);
	return id;
}

void GuildManager::DeleteGuild(Guild_t const &guild)
{
	m_Guilds.erase(guild->GetPawnId());
}

std::vector<GuildId_t> GuildManager::GetAllGuildIds() const
{
	std::vector<GuildId_t> guild_ids;
	for (auto &g : m_Guilds)
		guild_ids.push_back(g.first);

	return guild_ids;
}

Guild_t const &GuildManager::FindGuild(GuildId_t id)
{
	static Guild_t invalid_guild;
	auto it = m_Guilds.find(id);
	if (it == m_Guilds.end())
		return invalid_guild;
	return it->second;
}

Guild_t const &GuildManager::FindGuildByName(std::string const &name)
{
	static Guild_t invalid_guild;
	for (auto const &g : m_Guilds)
	{
		Guild_t const &guild = g.second;
		if (guild->GetName().compare(name) == 0)
			return guild;
	}
	return invalid_guild;
}

Guild_t const &GuildManager::FindGuildById(Snowflake_t const &sfid)
{
	static Guild_t invalid_guild;
	for (auto const &g : m_Guilds)
	{
		Guild_t const &guild = g.second;
		if (guild->GetId().compare(sfid) == 0)
			return guild;
	}
	return invalid_guild;
}
