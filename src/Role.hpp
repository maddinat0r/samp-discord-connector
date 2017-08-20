#pragma once

#include "CSingleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>

#include <json.hpp>


using json = nlohmann::json;


class Role
{
public:
	Role(RoleId_t pawn_id, json &data);
	~Role() = default;

private:
	const RoleId_t m_PawnId;

	Snowflake_t m_Id;

	std::string m_Name;
	int m_Color;
	bool m_Hoist;
	unsigned long long int m_Permissions;
	bool m_Mentionable;

public:
	inline RoleId_t GetPawnId() const
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
	inline int GetColor() const
	{
		return m_Color;
	}
	inline bool IsHoist() const
	{
		return m_Hoist;
	}
	inline decltype(m_Permissions) GetPermissions() const
	{
		return m_Permissions;
	}
	inline bool IsMentionable() const
	{
		return m_Mentionable;
	}

};


class RoleManager : public CSingleton<RoleManager>
{
	friend class CSingleton<RoleManager>;
private:
	RoleManager() = default;
	~RoleManager() = default;

private:
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };

	std::map<RoleId_t, Role_t> m_Roles; //PAWN role-id to actual channel map

public:
	Role_t const &AddRole(json &data);

	Role_t const &FindRole(RoleId_t id);
	Role_t const &FindRoleById(Snowflake_t const &sfid);
};
