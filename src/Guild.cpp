#include "Guild.hpp"
#include "Network.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include "Role.hpp"

#include <unordered_map>


Guild::Guild(GuildId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	m_Name = data["name"].get<std::string>();
	m_OwnerId = data["owner_id"].get<std::string>();

	for (auto &c : data["channels"])
		m_Channels.push_back(ChannelManager::Get()->AddChannel(c, pawn_id)->GetPawnId());

	for (auto &r : data["roles"])
		m_Roles.push_back(RoleManager::Get()->AddRole(r)->GetPawnId());


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
				// idle", "dnd", "online", or "offline
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


void GuildManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_ADD, [](json &data)
	{
		GuildManager::Get()->AddGuild(data["user"]);
	});

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

	// TODO: events
}

void GuildManager::WaitForInitialization()
{
	while (m_Initialized != m_InitValue)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
