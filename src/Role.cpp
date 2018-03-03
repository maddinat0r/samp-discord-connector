#include "Role.hpp"
#include "utils.hpp"


Role::Role(RoleId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	if (!utils::TryGetJsonValue(data, m_Id, "id"))
		return; // TODO: error log: invalid json

	Update(data);
}

void Role::Update(json &data)
{
	_valid =
		utils::TryGetJsonValue(data, m_Name, "name") &&
		utils::TryGetJsonValue(data, m_Color, "color") &&
		utils::TryGetJsonValue(data, m_Hoist, "hoist") &&
		utils::TryGetJsonValue(data, m_Permissions, "permissions") &&
		utils::TryGetJsonValue(data, m_Mentionable, "mentionable");

	if (!_valid)
	{
		// TODO: error log: invalid json
	}
}


RoleId_t RoleManager::AddRole(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
		return INVALID_ROLE_ID; // TODO: error msg: invalid json

	Role_t const &role = FindRoleById(sfid);
	if (role)
		return INVALID_ROLE_ID; // TODO: error log: role already exists

	RoleId_t id = 1;
	while (m_Roles.find(id) != m_Roles.end())
		++id;

	if (!m_Roles.emplace(id, Role_t(new Role(id, data))).first->second)
		return INVALID_ROLE_ID; // TODO: error log: duplicate key
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
