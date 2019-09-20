#pragma once

#include "Singleton.hpp"
#include "types.hpp"
#include "Callback.hpp"

#include <string>
#include <atomic>
#include <vector>
#include <unordered_set>

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
		ChannelId_t VoiceChannel = INVALID_CHANNEL_ID;


		void Update(json const &data);
		void UpdatePresence(std::string const &status);
		void UpdateVoiceChannel(ChannelId_t const &channel);

		inline ChannelId_t const &GetVoiceChannel() const
		{
			return VoiceChannel;
		}
	};

public:
	Guild(GuildId_t pawn_id, json const &data);
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
	std::unordered_set<UserId_t> m_MembersSet;

private:

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
		if (m_MembersSet.count(member.UserId) == 1)
			return;
		m_MembersSet.insert(member.UserId);
		m_Members.push_back(std::move(member));
	}
	inline void RemoveMember(UserId_t userid)
	{
		for (auto it = m_Members.begin(); it != m_Members.end(); it++)
		{
			if ((*it).UserId == userid)
			{
				m_Members.erase(it);
				m_MembersSet.erase(userid);
				break;
			}
		}
	}
	void UpdateMember(UserId_t userid, json const &data);
	void UpdateMemberPresence(UserId_t userid, std::string const &status);	
	void UpdateMemberVoiceChannel(UserId_t user_id, ChannelId_t const &channel);

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

	void Update(json const &data);

	void SetGuildName(std::string const &name);

	void SetMemberNickname(User_t const &user, std::string const &nickname);
	void SetMemberVoiceChannel(User_t const &user, Snowflake_t const &channel_id);
	void AddMemberRole(User_t const &user, Role_t const &role);
	void RemoveMemberRole(User_t const &user, Role_t const &role);
	void KickMember(User_t const &user);
	void CreateMemberBan(User_t const &user, std::string const &reason);
	void RemoveMemberBan(User_t const &user);

	void SetRolePosition(Role_t const &role, int position);
	void SetRoleName(Role_t const &role, std::string const &name);
	void SetRolePermissions(Role_t const &role, unsigned long long permissions);
	void SetRoleColor(Role_t const &role, unsigned int color);
	void SetRoleHoist(Role_t const &role, bool hoist);
	void SetRoleMentionable(Role_t const &role, bool mentionable);
	void DeleteRole(Role_t const &role);
};


class GuildManager : public Singleton<GuildManager>
{
	friend class Singleton<GuildManager>;
private:
	GuildManager() = default;
	~GuildManager() = default;

private:
	std::atomic<unsigned int>
		m_InitValue{ 1 },
		m_Initialized{ 0 };
	std::atomic<bool> m_IsInitialized{ false };

	std::map<GuildId_t, Guild_t> m_Guilds; //PAWN guild-id to actual guild map
	RoleId_t m_CreatedRoleId = INVALID_ROLE_ID;

private:
	GuildId_t AddGuild(json const &data);
	void DeleteGuild(Guild_t const &guild);

public:
	void Initialize();
	bool IsInitialized();

	bool CreateGuildRole(Guild_t const &guild,
		std::string const &name, pawn_cb::Callback_t &&callback);
	RoleId_t GetCreatedRoleChannelId() const
	{
		return m_CreatedRoleId;
	}
	std::vector<GuildId_t> GetAllGuildIds() const;

	Guild_t const &FindGuild(GuildId_t id);
	Guild_t const &FindGuildByName(std::string const &name);
	Guild_t const &FindGuildById(Snowflake_t const &sfid);
};