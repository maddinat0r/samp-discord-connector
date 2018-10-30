#include "Role.hpp"
#include "CLog.hpp"
#include "utils.hpp"


Role::Role(RoleId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	if (!utils::TryGetJsonValue(data, m_Id, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Update(data);
}

void Role::Update(json &data)
{
	_valid =
		utils::TryGetJsonValue(data, m_Name, "name") &&
		utils::TryGetJsonValue(data, m_Color, "color") &&
		utils::TryGetJsonValue(data, m_Hoist, "hoist") &&
		utils::TryGetJsonValue(data, m_Position, "position") &&
		utils::TryGetJsonValue(data, m_Permissions, "permissions") &&
		utils::TryGetJsonValue(data, m_Mentionable, "mentionable");

	if (!_valid)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't update role: invalid JSON: \"{}\"", data.dump());
	}
}


RoleId_t RoleManager::AddRole(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_ROLE_ID;
	}

	Role_t const &role = FindRoleById(sfid);
	if (role)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't add role: role id \"{}\" already exists (PAWN id '{}')",
			sfid, role->GetPawnId());
		return INVALID_ROLE_ID;
	}

	RoleId_t id = 1;
	while (m_Roles.find(id) != m_Roles.end())
		++id;

	if (!m_Roles.emplace(id, Role_t(new Role(id, data))).first->second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create role: duplicate key '{}'", id);
		return INVALID_ROLE_ID;
	}
	return id;
}

void RoleManager::RemoveRole(Role_t const &role)
{
	m_Roles.erase(role->GetPawnId());
}

Role_t const &RoleManager::FindRole(RoleId_t id)
{
	static Role_t invalid_role;
	auto it = m_Roles.find(id);
	if (it == m_Roles.end())
		return invalid_role;
	return it->second;
}

Role_t const &RoleManager::FindRoleById(Snowflake_t const &sfid)
{
	static Role_t invalid_role;
	for (auto const &g : m_Roles)
	{
		Role_t const &role = g.second;
		if (role->GetId().compare(sfid) == 0)
			return role;
	}
	return invalid_role;
}
