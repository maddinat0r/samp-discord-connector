#include "natives.hpp"
#include "Network.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include "Role.hpp"
#include "Guild.hpp"
#include "misc.hpp"
#include "types.hpp"
#include "CLog.hpp"

#include <fmt/printf.h>

#ifdef ERROR
#undef ERROR
#endif
/*
// native native_name(...);
AMX_DECLARE_NATIVE(Native::native_name)
{
	CScopedDebugInfo dbg_info(amx, "native_name", params, "sd");
	
	
	
	cell ret_val = ;
	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}
*/

// native DCC_Channel:DCC_FindChannelByName(const channel_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelByName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindChannelByName", params, "s");

	std::string const channel_name = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannelByName(channel_name);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Channel:DCC_FindChannelById(const channel_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelById)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindChannelById", params, "s");

	Snowflake_t const channel_id = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannelById(channel_id);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelId(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelId)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelId", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetId(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelType(DCC_Channel:channel, &DCC_ChannelType:type);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelType)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelType", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetType());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetChannelGuild(DCC_Channel:channel, &DCC_Guild:guild);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelGuild)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelGuild", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetGuildId());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetChannelName(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelName", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetName(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelTopic(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelTopic)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelTopic", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetTopic(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelPosition(DCC_Channel:channel, &position);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelPosition)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetChannelPosition", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetPosition());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsChannelNsfw(DCC_Channel:channel, &bool:is_nsfw);
AMX_DECLARE_NATIVE(Native::DCC_IsChannelNsfw)
{
	CScopedDebugInfo dbg_info(amx, "DCC_IsChannelNsfw", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	if (channel->GetType() != Channel::Type::GUILD_TEXT)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, 
			"invalid channel type; must be guild text channel");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->IsNsfw() ? 1 : 0);

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SendChannelMessage(DCC_Channel:channel, const message[]);
AMX_DECLARE_NATIVE(Native::DCC_SendChannelMessage)
{
	CScopedDebugInfo dbg_info(amx, "DCC_SendChannelMessage", params, "ds");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	channel->SendMessage(amx_GetCppString(amx, params[2]));

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_User:DCC_FindUserByName(const user_name[], const user_discriminator[]);
AMX_DECLARE_NATIVE(Native::DCC_FindUserByName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindUserByName", params, "ss");

	std::string const
		user_name = amx_GetCppString(amx, params[1]),
		discriminator = amx_GetCppString(amx, params[2]);
	User_t const &user = UserManager::Get()->FindUserByName(user_name, discriminator);

	cell ret_val = user ? user->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_User:DCC_FindUserById(const user_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindUserById)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindUserById", params, "s");

	Snowflake_t const user_id = amx_GetCppString(amx, params[1]);
	User_t const &user = UserManager::Get()->FindUserById(user_id);

	cell ret_val = user ? user->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserId(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserId)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetUserId", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetId(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserName(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetUserName", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetUsername(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserDiscriminator(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserDiscriminator)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetUserDiscriminator", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetDiscriminator(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_IsUserBot(DCC_User:user, &bool:is_bot);
AMX_DECLARE_NATIVE(Native::DCC_IsUserBot)
{
	CScopedDebugInfo dbg_info(amx, "DCC_IsUserBot", params, "dr");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = user->IsBot() ? 1 : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsUserVerified(DCC_User:user, &bool:is_verified);
AMX_DECLARE_NATIVE(Native::DCC_IsUserVerified)
{
	CScopedDebugInfo dbg_info(amx, "DCC_IsUserVerified", params, "dr");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = user->IsVerified() ? 1 : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Role:DCC_FindRoleByName(DCC_Guild:guild, const role_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindRoleByName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindRoleByName", params, "ds");

	GuildId_t guildid = params[1];
	std::string const role_name = amx_GetCppString(amx, params[2]);

	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return INVALID_ROLE_ID;
	}

	cell ret_val = INVALID_ROLE_ID;
	for (auto const &r : guild->GetRoles())
	{
		Role_t const &role = RoleManager::Get()->FindRole(r);
		if (!role)
			continue;

		if (role->GetName() == role_name)
		{
			ret_val = role->GetPawnId();
			break;
		}
	}

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Role:DCC_FindRoleById(const role_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindRoleById)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindRoleById", params, "s");

	Snowflake_t const role_id = amx_GetCppString(amx, params[1]);
	Role_t const &role = RoleManager::Get()->FindRoleById(role_id);

	cell ret_val = role ? role->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleId(DCC_Role:role, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleId)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetRoleId", params, "drd");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], role->GetId(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleName(DCC_Role:role, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetRoleName", params, "drd");

	ChannelId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], role->GetName(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleColor(DCC_Role:role, &color);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleColor)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetRoleColor", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(role->GetColor());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetRolePermissions(DCC_Role:role, &perm_high, &perm_low);
AMX_DECLARE_NATIVE(Native::DCC_GetRolePermissions)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetRolePermissions", params, "drr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference for 'perm_high'");
		return 0;
	}

	*dest = static_cast<cell>((role->GetPermissions() >> 32) & 0xFFFFFFFF);

	dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference for 'perm_low'");
		return 0;
	}

	*dest = static_cast<cell>(role->GetPermissions() & 0xFFFFFFFF);

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsRoleHoist(DCC_Role:role, &bool:is_hoist);
AMX_DECLARE_NATIVE(Native::DCC_IsRoleHoist)
{
	CScopedDebugInfo dbg_info(amx, "DCC_IsRoleHoist", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = role->IsHoist() ? 1 : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsRoleMentionable(DCC_Role:role, &bool:is_mentionable);
AMX_DECLARE_NATIVE(Native::DCC_IsRoleMentionable)
{
	CScopedDebugInfo dbg_info(amx, "DCC_IsRoleMentionable", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = role->IsMentionable() ? 1 : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Guild:DCC_FindGuildByName(const guild_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindGuildByName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindGuildByName", params, "s");

	std::string const guild_name = amx_GetCppString(amx, params[1]);
	Guild_t const &guild = GuildManager::Get()->FindGuildByName(guild_name);

	cell ret_val = guild ? guild->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Guild:DCC_FindGuildById(const guild_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindGuildById)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindGuildById", params, "s");

	Snowflake_t const guild_id = amx_GetCppString(amx, params[1]);
	Guild_t const &guild = GuildManager::Get()->FindGuildById(guild_id);

	cell ret_val = guild ? guild->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildId(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildId)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildId", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetId(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildName(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildName", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetName(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildOwnerId(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildOwnerId)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildOwnerId", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetOwnerId(), params[3]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildRole(DCC_Guild:guild, offset, &DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildRole)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildRole", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &roles = guild->GetRoles();
	if (offset >= roles.size())
	{
		CLog::Get()->LogNative(LogLevel::ERROR, 
			"invalid offset '{}', max size is '{}'",
			offset, roles.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles.at(offset));

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildRoleCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildRoleCount)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildRoleCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetRoles().size());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMember(DCC_Guild:guild, offset, &DCC_User:user);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMember)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMember", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &members = guild->GetMembers();
	if (offset >= members.size())
	{
		CLog::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, members.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(members.at(offset).UserId);

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberCount)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetMembers().size());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberNickname(DCC_Guild:guild, DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberNickname)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberNickname", params, "ddrd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	std::string nick;
	bool member_found = false;
	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != userid)
			continue;

		nick = m.Nickname;
		member_found = true;
		break;
	}

	if (!member_found)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[3], nick, params[4]) == AMX_ERR_NONE;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildMemberRole(DCC_Guild:guild, DCC_User:user, offset, &DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberRole)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberRole", params, "dddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	std::vector<RoleId_t> const *roles = nullptr;
	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != userid)
			continue;

		roles = &m.Roles;
		break;
	}

	if (roles == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[3]);
	if (offset >= roles->size())
	{
		CLog::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, roles->size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[4], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles->at(offset));

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberRoleCount(DCC_Guild:guild, DCC_User:user, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberRoleCount)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberRoleCount", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	std::vector<RoleId_t> const *roles = nullptr;
	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != userid)
			continue;

		roles = &m.Roles;
		break;
	}

	if (roles == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles->size());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_HasGuildMemberRole(DCC_Guild:guild, DCC_User:user, DCC_Role:role, &bool:has_role);
AMX_DECLARE_NATIVE(Native::DCC_HasGuildMemberRole)
{
	CScopedDebugInfo dbg_info(amx, "DCC_HasGuildMemberRole", params, "dddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	std::vector<RoleId_t> const *roles = nullptr;
	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != userid)
			continue;

		roles = &m.Roles;
		break;
	}

	if (roles == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[4], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	RoleId_t roleid = params[3];
	bool res = false;
	for (auto &r : *roles)
	{
		if (r == roleid)
		{
			res = true;
			break;
		}
	}

	*dest = res ? 1 : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberStatus(DCC_Guild:guild, DCC_User:user, &DCC_UserPresenceStatus:status);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberStatus)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberStatus", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	auto status = Guild::Member::PresenceStatus::INVALID;
	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != userid)
			continue;

		status = m.Status;
		break;
	}

	if (status == Guild::Member::PresenceStatus::INVALID)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(status);

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildChannel(DCC_Guild:guild, offset, &DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildChannel)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildChannel", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &channels = guild->GetChannels();
	if (offset >= channels.size())
	{
		CLog::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, channels.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channels.at(offset));

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildChannelCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildChannelCount)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetGuildChannelCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetChannels().size());

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetAllGuilds(DCC_Guild:dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetAllGuilds)
{
	CScopedDebugInfo dbg_info(amx, "DCC_GetAllGuilds", params, "rd");

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[1], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	auto guild_ids = GuildManager::Get()->GetAllGuildIds();

	cell const
		max_dest_size = params[2],
		guilds_count = static_cast<cell>(guild_ids.size());

	if (guilds_count > max_dest_size)
	{
		CLog::Get()->LogNative(LogLevel::WARNING,
			"destination array is too small (should be at least '{}' cells)",
			guilds_count);
	}

	cell const count = std::min(max_dest_size, guilds_count);
	for (cell i = 0; i != count; ++i)
		dest[i] = guild_ids.at(i);

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", count);
	return count;
}
