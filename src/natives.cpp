#include "natives.hpp"
#include "CCallback.hpp"
#include "CNetwork.hpp"
#include "CDispatcher.hpp"
#include "CMessage.hpp"
#include "CChannel.hpp"
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
	CScopedDebugInfo dbg_info(amx, "native_name", "sd");
	
	
	
	cell ret_val = ;
	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}
*/

// native DCC_Connect(const bot_token[]);
AMX_DECLARE_NATIVE(Native::DCC_Connect)
{
	CScopedDebugInfo dbg_info(amx, "DCC_Connect", "*");

	CChannelManager::Get()->Initialize(amx);
	CNetwork::Get()->Initialize(amx_GetCppString(amx, params[1]));

	CChannelManager::Get()->WaitForInitialization();

	cell ret_val = 1;
	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Channel:DCC_FindChannelByName(const channel_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelByName)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindChannelByName", "s");

	std::string const channel_name = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = CChannelManager::Get()->FindChannelByName(channel_name);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Channel:DCC_FindChannelById(const channel_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelById)
{
	CScopedDebugInfo dbg_info(amx, "DCC_FindChannelById", "s");

	Snowflake_t const channel_id = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = CChannelManager::Get()->FindChannelById(channel_id);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_SendChannelMessage(DCC_Channel:channel, const message[]);
AMX_DECLARE_NATIVE(Native::DCC_SendChannelMessage)
{
	CScopedDebugInfo dbg_info(amx, "DCC_SendMessage", "ds");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = CChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		CLog::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	channel->SendMessage(amx_GetCppString(amx, params[2]));

	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}
