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
		for (auto &mr : m["roles"])
		{
			Role_t const &role = RoleManager::Get()->FindRoleById(mr.get<std::string>());
			if (role)
				member.Roles.push_back(role->GetPawnId());
			else
			{
				// TODO: error message
			}
		}
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
				// "idle", "dnd", "online", or "offline"
				static const std::unordered_map<std::string, Member::PresenceStatus> status_map{
					{ "idle", Member::PresenceStatus::IDLE },
					{ "dnd", Member::PresenceStatus::DO_NOT_DISTURB },
					{ "online", Member::PresenceStatus::ONLINE },
					{ "offline", Member::PresenceStatus::OFFLINE }
				};

				m.Status = status_map.at(p["status"]);
				break;
			}
		}
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
		AddGuild(data);
		m_Initialized++;
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
	return true;
}

void GuildManager::AddGuild(json &data)
{
	Snowflake_t sfid = data["id"].get<std::string>();
	if (FindGuildById(sfid))
		return; // guild already exists

	GuildId_t id = 1;
	while (m_Guilds.find(id) != m_Guilds.end())
		++id;

	m_Guilds.emplace(id, Guild_t(new Guild(id, data)));
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
