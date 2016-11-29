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
	CScopedDebugInfo dbg_info(amx, "DCC_Connect", "s");

	CNetwork::Get()->WsRegisterEvent(CNetwork::WsEvent::MESSAGE_CREATE, 
		[amx](json &data)
		{
			CMessage msg(data);
			CDispatcher::Get()->Dispatch([amx, msg]()
			{
				CError<CCallback> cb_error;
				Callback_t cb = CCallback::Create(
					cb_error, amx, "DCC_OnMessageCreate", "sss",
					msg.GetAuthor().c_str(),
					msg.GetContent().c_str(),
					msg.GetChannelId().c_str());

				if (cb_error)
				{
					CLog::Get()->Log(LogLevel::ERROR, "{} error: {}",
						cb_error.module(), cb_error.msg());
				}
				else
				{
					cb->Execute();
				}
			});
		});
	CNetwork::Get()->Initialize(amx_GetCppString(amx, params[1]));
	CChannelManager::Get()->Initialize();

	cell ret_val = 1;
	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_SendMessage(const channel_id[], const message[]);
AMX_DECLARE_NATIVE(Native::DCC_SendMessage)
{
	CScopedDebugInfo dbg_info(amx, "DCC_SendMessage", "ss");

	std::string 
		channel_id = amx_GetCppString(amx, params[1]),
		message = amx_GetCppString(amx, params[2]);
	json data = {
		{ "content", message }
	};
	CNetwork::Get()->HttpPost("/channels/" + channel_id + "/messages", data.dump());

	cell ret_val = 1;
	CLog::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}
