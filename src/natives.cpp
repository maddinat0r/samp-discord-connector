#include "natives.hpp"
#include "CCallback.hpp"
#include "Network.hpp"
#include "CDispatcher.hpp"
#include "Message.hpp"
#include "Channel.hpp"
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
