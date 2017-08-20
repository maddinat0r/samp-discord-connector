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
		UserId_t UserId;
		std::vector<RoleId_t> Roles;
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

	std::map<GuildId_t, Guild_t> m_Guilds; //PAWN guild-id to actual channel map

private:
	void AddGuild(json &data);

public:
	void Initialize();
	void WaitForInitialization();

	Guild_t const &FindGuild(GuildId_t id);
	Guild_t const &FindGuildByName(std::string const &name);
	Guild_t const &FindGuildById(Snowflake_t const &sfid);
};
