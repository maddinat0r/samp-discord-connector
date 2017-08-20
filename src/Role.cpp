#include "Role.hpp"

Role::Role(RoleId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	m_Name = data["name"].get<std::string>();
	m_Color = data["color"].get<int>();
	m_Hoist = data["hoist"].get<bool>();
	m_Permissions = data["permissions"].get<decltype(m_Permissions)>();
	m_Mentionable = data["mentionable"].get<bool>();
}


Role_t const &RoleManager::AddRole(json &data)
{
	Snowflake_t sfid = data["id"].get<std::string>();
	Role_t const &role = FindRoleById(sfid);
	if (role)
		return role; // role already exists

	RoleId_t id = 1;
	while (m_Roles.find(id) != m_Roles.end())
		++id;

	return m_Roles.emplace(id, Role_t(new Role(id, data))).first->second;
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
