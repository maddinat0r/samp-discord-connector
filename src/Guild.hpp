#pragma once

#include "CSingleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>
#include <vector>

#include <json.hpp>


using json = nlohmann::json;


class Guild
{
public:
	struct Member
	{
		enum class PresenceStatus
		{
			INVALID = 0,
			ONLINE = 1,
			IDLE = 2,
			DO_NOT_DISTURB = 3,
			OFFLINE = 4
		};

		UserId_t UserId;
		std::string Nickname;
		std::vector<RoleId_t> Roles;
		PresenceStatus Status;
	};

public:
	Guild(GuildId_t pawn_id, json &data);
	~Guild() = default;

private:
	const GuildId_t m_PawnId;

	Snowflake_t m_Id;

	std::string
		m_Name;

	Snowflake_t m_OwnerId;

	std::vector<RoleId_t> m_Roles;
	std::vector<ChannelId_t> m_Channels;
	std::vector<Member> m_Members;

private:
	void UpdateMember(Member &member, json &data);

public:
	inline GuildId_t GetPawnId() const
	{
		return m_PawnId;
	}
	inline Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	inline std::string const &GetName() const
	{
		return m_Name;
	}
	inline Snowflake_t const &GetOwnerId() const
	{
		return m_OwnerId;
	}
	inline decltype(m_Roles) const &GetRoles() const
	{
		return m_Roles;
	}
	inline decltype(m_Members) const &GetMembers() const
	{
		return m_Members;
	}
	inline decltype(m_Channels) const &GetChannels() const
	{
		return m_Channels;
	}

	inline void AddChannel(ChannelId_t id)
	{
		m_Channels.push_back(id);
	}
	inline void RemoveChannel(ChannelId_t id)
	{
		for (auto it = m_Channels.begin(); it != m_Channels.end(); it++)
		{
			if (*it == id)
			{
				m_Channels.erase(it);
				break;
			}
		}
	}

	inline void AddMember(Member &&member)
	{
		m_Members.push_back(std::move(member));
	}
	inline void RemoveMember(UserId_t userid)
	{
		for (auto it = m_Members.begin(); it != m_Members.end(); it++)
		{
			if ((*it).UserId == userid)
			{
				m_Members.erase(it);
				break;
			}
		}
	}
	void UpdateMember(UserId_t userid, json &data);

	inline void AddRole(RoleId_t id)
	{
		m_Roles.push_back(id);
	}
	inline void RemoveRole(RoleId_t id)
	{
		for (auto it = m_Roles.begin(); it != m_Roles.end(); it++)
		{
			if (*it == id)
			{
				m_Roles.erase(it);
				break;
			}
		}
	}

	void Update(json &data);
};


class GuildManager : public CSingleton<GuildManager>
{
	friend class CSingleton<GuildManager>;
private:
	GuildManager() = default;
	~GuildManager() = default;

private:
	std::atomic<unsigned int> 
		m_InitValue{ 1 },
		m_Initialized{ 0 };
	std::atomic<bool> m_IsInitialized{ false };

	std::map<GuildId_t, Guild_t> m_Guilds; //PAWN guild-id to actual channel map

private:
	Guild_t const &AddGuild(json &data);
	void DeleteGuild(Guild_t const &guild);

public:
	void Initialize();
	bool WaitForInitialization();

	Guild_t const &FindGuild(GuildId_t id);
	Guild_t const &FindGuildByName(std::string const &name);
	Guild_t const &FindGuildById(Snowflake_t const &sfid);
};
