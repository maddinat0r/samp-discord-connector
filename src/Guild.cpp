#include "Guild.hpp"
#include "Network.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include "Role.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"

#include <unordered_map>


Guild::Guild(GuildId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();

	Update(data);

	for (auto &c : data["channels"])
	{
		auto const &channel = ChannelManager::Get()->AddChannel(c, pawn_id);
		if (channel)
			m_Channels.push_back(channel->GetPawnId());
	}

	for (auto &m : data["members"])
	{
		Member member;
		member.UserId = UserManager::Get()->AddUser(m["user"])->GetPawnId();
		UpdateMember(member, m);
		m_Members.push_back(std::move(member));
	}

	for (auto &p : data["presences"])
	{
		Snowflake_t userid = p["user"]["id"].get<std::string>();
		for (auto &m : m_Members)
		{
			User_t const &user = UserManager::Get()->FindUser(m.UserId);
			assert(user);
			if (user->GetId() == userid)
			{
				UpdateMemberPresence(m, p["status"].get<std::string>());
				break;
			}
		}
	}
}

void Guild::UpdateMember(Member &member, json &data)
{
	// we don't care about the user object, there's an extra event for users
	member.Roles.clear();
	for (auto &mr : data["roles"])
	{
		Role_t const &role = RoleManager::Get()->FindRoleById(mr.get<std::string>());
		if (role)
		{
			member.Roles.push_back(role->GetPawnId());
		}
		else
		{
			// TODO: error message
		}
	}

	if (!data["nick"].is_null())
		member.Nickname = data["nick"].get<std::string>();
	else
		member.Nickname.clear();
}

void Guild::UpdateMemberPresence(Member &member, std::string const &status)
{
	// "idle", "dnd", "online", or "offline"
	static const std::unordered_map<std::string, Member::PresenceStatus> status_map{
		{ "idle", Member::PresenceStatus::IDLE },
		{ "dnd", Member::PresenceStatus::DO_NOT_DISTURB },
		{ "online", Member::PresenceStatus::ONLINE },
		{ "offline", Member::PresenceStatus::OFFLINE }
	};

	member.Status = status_map.at(status);
}

void Guild::UpdateMember(UserId_t userid, json &data)
{
	for (auto &m : m_Members)
	{
		if (m.UserId != userid)
			continue;

		UpdateMember(m, data);
		break;
	}
}

void Guild::UpdateMemberPresence(UserId_t userid, std::string const &status)
{
	for (auto &m : m_Members)
	{
		if (m.UserId != userid)
			continue;

		UpdateMemberPresence(m, status);
		break;
	}
}

void Guild::Update(json &data)
{
	m_Name = data["name"].get<std::string>();
	m_OwnerId = data["owner_id"].get<std::string>();

	for (auto &r : data["roles"])
	{
		auto const &role = RoleManager::Get()->FindRoleById(r["id"].get<std::string>());
		if (role)
			role->Update(r);
		else
			m_Roles.push_back(RoleManager::Get()->AddRole(r)->GetPawnId());
	}
}


void GuildManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
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
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t sfid = data["id"].get<std::string>();
			Guild_t const &guild = GuildManager::Get()->FindGuildById(sfid);
			if (!guild)
				return; // TODO: warning msg: guild isn't cached, it probably should have

			// forward DCC_OnGuildDelete(DCC_Guild:guild);
			PawnCallbackManager::Get()->Call("DCC_OnGuildDelete", guild->GetPawnId());

			GuildManager::Get()->DeleteGuild(guild);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t sfid = data["id"].get<std::string>();
			Guild_t const &guild = GuildManager::Get()->FindGuildById(sfid);
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			guild->Update(data);

			// forward DCC_OnGuildUpdate(DCC_Guild:guild);
			PawnCallbackManager::Get()->Call("DCC_OnGuildUpdate", guild->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_ADD, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			Guild::Member member;
			// returns correct user if he already exists
			auto const userid = UserManager::Get()->AddUser(data["user"]);
			member.UserId = userid;
			for (auto &mr : data["roles"])
			{
				Role_t const &role = RoleManager::Get()->FindRoleById(mr.get<std::string>());
				if (role)
					member.Roles.push_back(role->GetPawnId());
				else
				{
					// TODO: error message
				}
			}

			guild->AddMember(std::move(member));

			// forward DCC_OnGuildMemberAdd(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberAdd", guild->GetPawnId(), userid);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_REMOVE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const &user = UserManager::Get()->FindUserById(data["user"]["id"].get<std::string>());
			if (!user)
				return; // TODO: error msg: user not cached (cache mismatch)

			// forward DCC_OnGuildMemberRemove(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberRemove", guild->GetPawnId(), user->GetPawnId());

			guild->RemoveMember(user->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const &user = UserManager::Get()->FindUserById(data["user"]["id"].get<std::string>());
			if (!user)
				return; // TODO: error msg: user not cached (cache mismatch)

			guild->UpdateMember(user->GetPawnId(), data);

			// forward DCC_OnGuildMemberUpdate(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberUpdate", guild->GetPawnId(), user->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_CREATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const role = RoleManager::Get()->AddRole(data["role"]);
			guild->AddRole(role);

			// forward DCC_OnGuildRoleCreate(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleCreate", guild->GetPawnId(), role);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_DELETE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const &role = RoleManager::Get()->FindRoleById(data["role_id"].get<std::string>());
			if (!role)
				return; // TODO: error msg: role isn't cached (cache mismatch)

			// forward DCC_OnGuildRoleDelete(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleDelete", guild->GetPawnId(), role->GetPawnId());

			guild->RemoveRole(role->GetPawnId());
			RoleManager::Get()->RemoveRole(role);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_ROLE_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &guild = GuildManager::Get()->FindGuildById(data["guild_id"].get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const &role = RoleManager::Get()->FindRoleById(data["role"]["id"].get<std::string>());
			if (!role)
				return; // TODO: error msg: role isn't cached (cache mismatch)

			role->Update(data["role"]);

			// forward DCC_OnGuildRoleUpdate(DCC_Guild:guild, DCC_Role:role);
			PawnCallbackManager::Get()->Call("DCC_OnGuildRoleUpdate", guild->GetPawnId(), role->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::PRESENCE_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto json_guildid_it = data.find("guild_id");
			if (json_guildid_it == data.end())
				return;

			json &json_guildid = *json_guildid_it;
			if (json_guildid.is_null())
				return;

			auto json_user_it = data.find("user");
			if (json_user_it == data.end())
				return;

			json &json_user = *json_user_it;
			if (json_user.is_null())
				return;

			auto json_user_id_it = json_user.find("id");
			if (json_user_id_it == json_user.end())
				return;

			json &json_user_id = *json_user_id_it;
			if (json_user_id.is_null())
				return;

			auto json_status_it = data.find("status");
			if (json_status_it == data.end())
				return;

			json &json_status = *json_status_it;
			if (json_status.is_null())
				return;

			auto const &guild = GuildManager::Get()->FindGuildById(json_guildid.get<std::string>());
			if (!guild)
				return; // TODO: error msg: guild isn't cached, it probably should have

			auto const &user = UserManager::Get()->FindUserById(json_user_id.get<std::string>());
			if (!user)
				return; // TODO: error msg: user not cached (cache mismatch)

			guild->UpdateMemberPresence(user->GetPawnId(), json_status.get<std::string>());

			// forward DCC_OnGuildMemberUpdate(DCC_Guild:guild, DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnGuildMemberUpdate", guild->GetPawnId(), user->GetPawnId());
		});
	});

	// TODO: events
}

bool GuildManager::WaitForInitialization()
{
	unsigned int const
		SLEEP_TIME_MS = 20,
		TIMEOUT_TIME_MS = 20 * 1000;
	unsigned int waited_time = 0;
	while (m_Initialized != m_InitValue)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
		waited_time += SLEEP_TIME_MS;
		if (waited_time > TIMEOUT_TIME_MS)
			return false;
	}
	m_IsInitialized = true;
	return true;
}

GuildId_t GuildManager::AddGuild(json &data)
{
	Snowflake_t sfid = data["id"].get<std::string>();
	auto const &guild = FindGuildById(sfid);
	if (guild)
		return INVALID_GUILD_ID; // TODO: error log: guild already exists

	GuildId_t id = 1;
	while (m_Guilds.find(id) != m_Guilds.end())
		++id;

	if (!m_Guilds.emplace(id, Guild_t(new Guild(id, data))).second)
		return INVALID_GUILD_ID; // TODO: error log: duplicate key

	return id;
}

void GuildManager::DeleteGuild(Guild_t const &guild)
{
	m_Guilds.erase(guild->GetPawnId());
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
