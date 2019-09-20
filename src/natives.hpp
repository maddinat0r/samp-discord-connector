#include "natives.hpp"
#include "Network.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "User.hpp"
#include "Role.hpp"
#include "Guild.hpp"
#include "Bot.hpp"
#include "misc.hpp"
#include "types.hpp"
#include "Logger.hpp"
#include "Callback.hpp"

#include <fmt/printf.h>

#ifdef ERROR
#undef ERROR
#endif
/*
// native native_name(...);
AMX_DECLARE_NATIVE(Native::native_name)
{
	ScopedDebugInfo dbg_info(amx, "native_name", params, "sd");



	cell ret_val = ;
	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}
*/

// native DCC_Channel:DCC_FindChannelByName(const channel_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelByName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindChannelByName", params, "s");

	std::string const channel_name = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannelByName(channel_name);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Channel:DCC_FindChannelById(const channel_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindChannelById)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindChannelById", params, "s");

	Snowflake_t const channel_id = amx_GetCppString(amx, params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannelById(channel_id);

	cell ret_val = channel ? channel->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelId(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelId", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelType(DCC_Channel:channel, &DCC_ChannelType:type);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelType)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelType", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetType());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetChannelGuild(DCC_Channel:channel, &DCC_Guild:guild);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelGuild)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelGuild", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetGuildId());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetChannelName(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelName", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetName(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelTopic(DCC_Channel:channel, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelTopic)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelTopic", params, "drd");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], channel->GetTopic(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetChannelPosition(DCC_Channel:channel, &position);
AMX_DECLARE_NATIVE(Native::DCC_GetChannelPosition)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetChannelPosition", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->GetPosition());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsChannelNsfw(DCC_Channel:channel, &bool:is_nsfw);
AMX_DECLARE_NATIVE(Native::DCC_IsChannelNsfw)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsChannelNsfw", params, "dr");

	ChannelId_t channelid = params[1];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	if (channel->GetType() != Channel::Type::GUILD_TEXT)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid channel type; must be guild text channel");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channel->IsNsfw() ? 1 : 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SendChannelMessage(DCC_Channel:channel, const message[], 
//     const callback[] = "", const format[] = "", {Float, _}:...);
AMX_DECLARE_NATIVE(Native::DCC_SendChannelMessage)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SendChannelMessage", params, "ds");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	auto message = amx_GetCppString(amx, params[2]);
	if (message.length() > 2000)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"message must be shorter than 2000 characters");
		return 0;
	}

	auto
		cb_name = amx_GetCppString(amx, params[3]),
		cb_format = amx_GetCppString(amx, params[4]);

	pawn_cb::Error cb_error;
	auto cb = pawn_cb::Callback::Prepare(
		amx, cb_name.c_str(), cb_format.c_str(), params, 5, cb_error);
	if (cb_error && cb_error.get() != pawn_cb::Error::Type::EMPTY_NAME)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "could not prepare callback");
		return 0;
	}


	channel->SendMessage(std::move(message), std::move(cb));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetChannelName(DCC_Channel:channel, const name[]);
AMX_DECLARE_NATIVE(Native::DCC_SetChannelName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetChannelName", params, "ds");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	auto name = amx_GetCppString(amx, params[2]);
	if (name.length() < 2 || name.length() > 100)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"name must be between 2 and 100 characters in length");
		return 0;
	}

	channel->SetChannelName(name);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetChannelTopic(DCC_Channel:channel, const topic[]);
AMX_DECLARE_NATIVE(Native::DCC_SetChannelTopic)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetChannelTopic", params, "ds");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	if (channel->GetType() != Channel::Type::GUILD_TEXT)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid channel type; must be guild text channel");
		return 0;
	}

	auto topic = amx_GetCppString(amx, params[2]);
	if (topic.length() > 1024)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"topic must be between 0 and 1024 characters in length");
		return 0;
	}

	channel->SetChannelTopic(topic);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetChannelPosition(DCC_Channel:channel, position);
AMX_DECLARE_NATIVE(Native::DCC_SetChannelPosition)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetChannelPosition", params, "dd");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	channel->SetChannelPosition(static_cast<int>(params[2]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetChannelNsfw(DCC_Channel:channel, bool:is_nsfw);
AMX_DECLARE_NATIVE(Native::DCC_SetChannelNsfw)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetChannelNsfw", params, "dd");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	channel->SetChannelNsfw(params[2] != 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetChannelParentCategory(DCC_Channel:channel, DCC_Channel:parent_category);
AMX_DECLARE_NATIVE(Native::DCC_SetChannelParentCategory)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetChannelParentCategory", params, "dd");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	ChannelId_t parent_channelid = static_cast<ChannelId_t>(params[2]);
	Channel_t const &parent_channel = ChannelManager::Get()->FindChannel(parent_channelid);
	if (!parent_channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid parent channel id '{}'",
			parent_channelid);
		return 0;
	}

	if (parent_channel->GetType() != Channel::Type::GUILD_CATEGORY)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid channel type; must be guild category channel");
		return 0;
	}

	channel->SetChannelParentCategory(parent_channel);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_DeleteChannel(DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_DeleteChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_DeleteChannel", params, "d");

	ChannelId_t channelid = static_cast<ChannelId_t>(params[1]);
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	channel->DeleteChannel();

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageId(DCC_Message:message, dest[DCC_ID_SIZE], max_size = DCC_ID_SIZE);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageId", params, "drd");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], msg->GetId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetMessageChannel(DCC_Message:message, &DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageChannel", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->GetChannel());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageAuthor(DCC_Message:message, &DCC_User:author);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageAuthor)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageAuthor", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->GetAuthor());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageContent(DCC_Message:message, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageContent)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageContent", params, "drd");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell ret_val = amx_SetCppString(
		amx, params[2], msg->GetContent(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_IsMessageTts(DCC_Message:message, &bool:is_tts);
AMX_DECLARE_NATIVE(Native::DCC_IsMessageTts)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsMessageTts", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->IsTts() ? 1 : 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsMessageMentioningEveryone(DCC_Message:message, &bool:mentions_everyone);
AMX_DECLARE_NATIVE(Native::DCC_IsMessageMentioningEveryone)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsMessageMentioningEveryone", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->MentionsEveryone() ? 1 : 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageUserMentionCount(DCC_Message:message, &mentioned_user_count);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageUserMentionCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageUserMentionCount", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->GetUserMentions().size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageUserMention(DCC_Message:message, offset, &DCC_User:mentioned_user);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageUserMention)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageUserMention", params, "ddr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	auto const offset = static_cast<unsigned int>(params[2]);
	auto const &mentions = msg->GetUserMentions();
	if (offset >= mentions.size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, mentions.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(mentions.at(offset));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageRoleMentionCount(DCC_Message:message, &mentioned_role_count);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageRoleMentionCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageRoleMentionCount", params, "dr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(msg->GetRoleMentions().size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetMessageRoleMention(DCC_Message:message, offset, &DCC_Role:mentioned_role);
AMX_DECLARE_NATIVE(Native::DCC_GetMessageRoleMention)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetMessageRoleMention", params, "ddr");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	auto const offset = static_cast<unsigned int>(params[2]);
	auto const &mentions = msg->GetRoleMentions();
	if (offset >= mentions.size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, mentions.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(mentions.at(offset));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_DeleteMessage(DCC_Message:message);
AMX_DECLARE_NATIVE(Native::DCC_DeleteMessage)
{
	ScopedDebugInfo dbg_info(amx, "DCC_DeleteMessage", params, "d");

	MessageId_t id = params[1];
	Message_t const &msg = MessageManager::Get()->Find(id);
	if (!msg)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid message id '{}'", id);
		return 0;
	}

	msg->DeleteMessage();

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Message:DCC_GetCreatedMessage();
AMX_DECLARE_NATIVE(Native::DCC_GetCreatedMessage)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetCreatedMessage", params);

	auto ret_val = static_cast<cell>(MessageManager::Get()->GetCreatedMessageId());
	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{:d}'", ret_val);
	return ret_val;
}

// native DCC_User:DCC_FindUserByName(const user_name[], const user_discriminator[]);
AMX_DECLARE_NATIVE(Native::DCC_FindUserByName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindUserByName", params, "ss");

	std::string const
		user_name = amx_GetCppString(amx, params[1]),
		discriminator = amx_GetCppString(amx, params[2]);
	User_t const &user = UserManager::Get()->FindUserByName(user_name, discriminator);

	cell ret_val = user ? user->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_User:DCC_FindUserById(const user_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindUserById)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindUserById", params, "s");

	Snowflake_t const user_id = amx_GetCppString(amx, params[1]);
	User_t const &user = UserManager::Get()->FindUserById(user_id);

	cell ret_val = user ? user->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserId(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetUserId", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserName(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetUserName", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetUsername(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetUserDiscriminator(DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetUserDiscriminator)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetUserDiscriminator", params, "drd");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], user->GetDiscriminator(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_IsUserBot(DCC_User:user, &bool:is_bot);
AMX_DECLARE_NATIVE(Native::DCC_IsUserBot)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsUserBot", params, "dr");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = user->IsBot() ? 1 : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsUserVerified(DCC_User:user, &bool:is_verified);
AMX_DECLARE_NATIVE(Native::DCC_IsUserVerified)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsUserVerified", params, "dr");

	UserId_t userid = params[1];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = user->IsVerified() ? 1 : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Role:DCC_FindRoleByName(DCC_Guild:guild, const role_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindRoleByName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindRoleByName", params, "ds");

	GuildId_t guildid = params[1];
	std::string const role_name = amx_GetCppString(amx, params[2]);

	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Role:DCC_FindRoleById(const role_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindRoleById)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindRoleById", params, "s");

	Snowflake_t const role_id = amx_GetCppString(amx, params[1]);
	Role_t const &role = RoleManager::Get()->FindRoleById(role_id);

	cell ret_val = role ? role->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleId(DCC_Role:role, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetRoleId", params, "drd");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], role->GetId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleName(DCC_Role:role, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetRoleName", params, "drd");

	ChannelId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], role->GetName(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetRoleColor(DCC_Role:role, &color);
AMX_DECLARE_NATIVE(Native::DCC_GetRoleColor)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetRoleColor", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(role->GetColor());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetRolePermissions(DCC_Role:role, &perm_high, &perm_low);
AMX_DECLARE_NATIVE(Native::DCC_GetRolePermissions)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetRolePermissions", params, "drr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference for 'perm_high'");
		return 0;
	}

	*dest = static_cast<cell>((role->GetPermissions() >> 32) & 0xFFFFFFFF);

	dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference for 'perm_low'");
		return 0;
	}

	*dest = static_cast<cell>(role->GetPermissions() & 0xFFFFFFFF);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsRoleHoist(DCC_Role:role, &bool:is_hoist);
AMX_DECLARE_NATIVE(Native::DCC_IsRoleHoist)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsRoleHoist", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = role->IsHoist() ? 1 : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetRolePosition(DCC_Role:role, &position);
AMX_DECLARE_NATIVE(Native::DCC_GetRolePosition)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetRolePosition", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(role->GetPosition());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_IsRoleMentionable(DCC_Role:role, &bool:is_mentionable);
AMX_DECLARE_NATIVE(Native::DCC_IsRoleMentionable)
{
	ScopedDebugInfo dbg_info(amx, "DCC_IsRoleMentionable", params, "dr");

	RoleId_t roleid = params[1];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = role->IsMentionable() ? 1 : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Guild:DCC_FindGuildByName(const guild_name[]);
AMX_DECLARE_NATIVE(Native::DCC_FindGuildByName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindGuildByName", params, "s");

	std::string const guild_name = amx_GetCppString(amx, params[1]);
	Guild_t const &guild = GuildManager::Get()->FindGuildByName(guild_name);

	cell ret_val = guild ? guild->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_Guild:DCC_FindGuildById(const guild_id[]);
AMX_DECLARE_NATIVE(Native::DCC_FindGuildById)
{
	ScopedDebugInfo dbg_info(amx, "DCC_FindGuildById", params, "s");

	Snowflake_t const guild_id = amx_GetCppString(amx, params[1]);
	Guild_t const &guild = GuildManager::Get()->FindGuildById(guild_id);

	cell ret_val = guild ? guild->GetPawnId() : 0;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildId(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildId", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildName(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildName", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetName(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildOwnerId(DCC_Guild:guild, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildOwnerId)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildOwnerId", params, "drd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[2], guild->GetOwnerId(), params[3]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildRole(DCC_Guild:guild, offset, &DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildRole", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &roles = guild->GetRoles();
	if (offset >= roles.size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, roles.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles.at(offset));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildRoleCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildRoleCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildRoleCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetRoles().size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMember(DCC_Guild:guild, offset, &DCC_User:user);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMember)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMember", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &members = guild->GetMembers();
	if (offset >= members.size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, members.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(members.at(offset).UserId);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetMembers().size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberVoiceChannel(DCC_Guild:guild, DCC_User:user, &DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberVoiceChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberVoiceChannel", params, "ddr");

	GuildId_t guild_id = params[1];
	auto const &guild = GuildManager::Get()->FindGuild(guild_id);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guild_id);
		return 0;
	}

	UserId_t user_id = params[2];
	/*auto const &user = Guild::Get()->FindUser(user_id);
	if (!user)
	{
		
	}*/

	for (auto &m : guild->GetMembers())
	{
		if (m.UserId != user_id)
			continue;

		cell *dest = nullptr;
		if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
		{
			Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
			return 0;
		}
		//static_cast<cell>(channels.at(offset));

		

		*dest = static_cast<cell>(m.GetVoiceChannel());
		Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
		return 1;
	}

	Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", user_id);
	return 0;
}

// native DCC_GetGuildMemberNickname(DCC_Guild:guild, DCC_User:user, dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberNickname)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberNickname", params, "ddrd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell ret_val = amx_SetCppString(amx, params[3], nick, params[4]) == AMX_ERR_NONE;

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_GetGuildMemberRole(DCC_Guild:guild, DCC_User:user, offset, &DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberRole", params, "dddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[3]);
	if (offset >= roles->size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, roles->size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[4], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles->at(offset));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberRoleCount(DCC_Guild:guild, DCC_User:user, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberRoleCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberRoleCount", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(roles->size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_HasGuildMemberRole(DCC_Guild:guild, DCC_User:user, DCC_Role:role, &bool:has_role);
AMX_DECLARE_NATIVE(Native::DCC_HasGuildMemberRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_HasGuildMemberRole", params, "dddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[4], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
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

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildMemberStatus(DCC_Guild:guild, DCC_User:user, &DCC_UserPresenceStatus:status);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildMemberStatus)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildMemberStatus", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
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
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user specified");
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(status);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildChannel(DCC_Guild:guild, offset, &DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildChannel", params, "ddr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto offset = static_cast<unsigned int>(params[2]);
	auto const &channels = guild->GetChannels();
	if (offset >= channels.size())
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"invalid offset '{}', max size is '{}'",
			offset, channels.size());
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[3], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(channels.at(offset));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetGuildChannelCount(DCC_Guild:guild, &count);
AMX_DECLARE_NATIVE(Native::DCC_GetGuildChannelCount)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetGuildChannelCount", params, "dr");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	*dest = static_cast<cell>(guild->GetChannels().size());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_GetAllGuilds(DCC_Guild:dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_GetAllGuilds)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetAllGuilds", params, "rd");

	cell *dest = nullptr;
	if (amx_GetAddr(amx, params[1], &dest) != AMX_ERR_NONE || dest == nullptr)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid reference");
		return 0;
	}

	auto guild_ids = GuildManager::Get()->GetAllGuildIds();

	cell const
		max_dest_size = params[2],
		guilds_count = static_cast<cell>(guild_ids.size());

	if (guilds_count > max_dest_size)
	{
		Logger::Get()->LogNative(LogLevel::WARNING,
			"destination array is too small (should be at least '{}' cells)",
			guilds_count);
	}

	cell const count = std::min(max_dest_size, guilds_count);
	for (cell i = 0; i != count; ++i)
		dest[i] = guild_ids.at(i);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", count);
	return count;
}

// native DCC_SetGuildName(DCC_Guild:guild, const name[]);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildName", params, "ds");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto name = amx_GetCppString(amx, params[2]);
	if (name.length() < 2 || name.length() > 100)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"name must be between 2 and 100 characters in length");
		return 0;
	}

	guild->SetGuildName(name);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_CreateGuildChannel(DCC_Guild:guild, const name[], DCC_ChannelType:type,
//     const callback[], const format[], {Float, _}:...);
AMX_DECLARE_NATIVE(Native::DCC_CreateGuildChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_CreateGuildChannel", params, "dsdss");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto name = amx_GetCppString(amx, params[2]);
	if (name.length() < 2 || name.length() > 100)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"name must be between 2 and 100 characters in length");
		return 0;
	}

	auto type = static_cast<Channel::Type>(params[3]);
	if (type != Channel::Type::GUILD_CATEGORY
		&& type != Channel::Type::GUILD_TEXT
		&& type != Channel::Type::GUILD_VOICE)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel type");
		return 0;
	}

	auto
		cb_name = amx_GetCppString(amx, params[4]),
		cb_format = amx_GetCppString(amx, params[5]);

	pawn_cb::Error cb_error;
	auto cb = pawn_cb::Callback::Prepare(
		amx, cb_name.c_str(), cb_format.c_str(), params, 6, cb_error);
	if (cb_error && cb_error.get() != pawn_cb::Error::Type::EMPTY_NAME)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "could not prepare callback");
		return 0;
	}

	ChannelManager::Get()->CreateGuildChannel(guild, name, type, std::move(cb));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Channel:DCC_GetCreatedGuildChannel();
AMX_DECLARE_NATIVE(Native::DCC_GetCreatedGuildChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetCreatedGuildChannel", params);
	return ChannelManager::Get()->GetCreatedGuildChannelId();
}

// native DCC_SetGuildMemberNickname(DCC_Guild:guild, DCC_User:user, const nickname[]);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildMemberNickname)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildMemberNickname", params, "dds");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	guild->SetMemberNickname(user, amx_GetCppString(amx, params[3]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildMemberVoiceChannel(DCC_Guild:guild, DCC_User:user, DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildMemberVoiceChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildMemberVoiceChannel", params, "ddd");

	GuildId_t guild_id = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guild_id);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guild_id);
		return 0;
	}

	UserId_t user_id = params[2];
	User_t const &user = UserManager::Get()->FindUser(user_id);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", user_id);
		return 0;
	}

	ChannelId_t channel_id = params[3];
	Channel_t const &channel = ChannelManager::Get()->FindChannel(channel_id);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channel_id);
		return 0;
	}
	guild->SetMemberVoiceChannel(user, channel->GetId());

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_AddGuildMemberRole(DCC_Guild:guild, DCC_User:user, DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_AddGuildMemberRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_AddGuildMemberRole", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	RoleId_t roleid = params[3];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->AddMemberRole(user, role);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_RemoveGuildMemberRole(DCC_Guild:guild, DCC_User:user, DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_RemoveGuildMemberRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_RemoveGuildMemberRole", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	RoleId_t roleid = params[3];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->RemoveMemberRole(user, role);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_RemoveGuildMember(DCC_Guild:guild, DCC_User:user);
AMX_DECLARE_NATIVE(Native::DCC_RemoveGuildMember)
{
	ScopedDebugInfo dbg_info(amx, "DCC_RemoveGuildMember", params, "dd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	guild->KickMember(user);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_CreateGuildMemberBan(DCC_Guild:guild, DCC_User:user, const reason[] = "");
AMX_DECLARE_NATIVE(Native::DCC_CreateGuildMemberBan)
{
	ScopedDebugInfo dbg_info(amx, "DCC_CreateGuildMemberBan", params, "dds");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	guild->CreateMemberBan(user, amx_GetCppString(amx, params[3]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_RemoveGuildMemberBan(DCC_Guild:guild, DCC_User:user);
AMX_DECLARE_NATIVE(Native::DCC_RemoveGuildMemberBan)
{
	ScopedDebugInfo dbg_info(amx, "DCC_RemoveGuildMemberBan", params, "dd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	UserId_t userid = params[2];
	User_t const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	guild->RemoveMemberBan(user);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRolePosition(DCC_Guild:guild, DCC_Role:role, position);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRolePosition)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRolePosition", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->SetRolePosition(role, static_cast<int>(params[3]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRoleName(DCC_Guild:guild, DCC_Role:role, const name[]);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRoleName)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRoleName", params, "dds");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->SetRoleName(role, amx_GetCppString(amx, params[3]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRolePermissions(DCC_Guild:guild, DCC_Role:role, perm_high, perm_low);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRolePermissions)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRolePermissions", params, "dddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	unsigned long long
		perm_high = static_cast<unsigned long long>(params[3]) << 32,
		perm_low = static_cast<unsigned long long>(params[4]),
		permissions = perm_high & perm_low;

	guild->SetRolePermissions(role, permissions);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRoleColor(DCC_Guild:guild, DCC_Role:role, color);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRoleColor)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRoleColor", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->SetRoleColor(role, static_cast<unsigned int>(params[3]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRoleHoist(DCC_Guild:guild, DCC_Role:role, bool:hoist);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRoleHoist)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRoleColor", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->SetRoleHoist(role, params[3] != 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_SetGuildRoleMentionable(DCC_Guild:guild, DCC_Role:role, bool:mentionable);
AMX_DECLARE_NATIVE(Native::DCC_SetGuildRoleMentionable)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetGuildRoleColor", params, "ddd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->SetRoleMentionable(role, params[3] != 0);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_CreateGuildRole(DCC_Guild:guild, const name[],
//     const callback[], const format[], {Float, _}:...);
AMX_DECLARE_NATIVE(Native::DCC_CreateGuildRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_CreateGuildRole", params, "dsss");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto name = amx_GetCppString(amx, params[2]);
	if (name.length() < 2 || name.length() > 100)
	{
		Logger::Get()->LogNative(LogLevel::ERROR,
			"name must be between 2 and 100 characters in length");
		return 0;
	}

	auto
		cb_name = amx_GetCppString(amx, params[3]),
		cb_format = amx_GetCppString(amx, params[4]);

	pawn_cb::Error cb_error;
	auto cb = pawn_cb::Callback::Prepare(
		amx, cb_name.c_str(), cb_format.c_str(), params, 5, cb_error);
	if (cb_error && cb_error.get() != pawn_cb::Error::Type::EMPTY_NAME)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "could not prepare callback");
		return 0;
	}

	GuildManager::Get()->CreateGuildRole(guild, name, std::move(cb));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Role:DCC_GetCreatedGuildRole();
AMX_DECLARE_NATIVE(Native::DCC_GetCreatedGuildRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetCreatedGuildRole", params);
	return GuildManager::Get()->GetCreatedRoleChannelId();
}

// native DCC_DeleteGuildRole(DCC_Guild:guild, DCC_Role:role);
AMX_DECLARE_NATIVE(Native::DCC_DeleteGuildRole)
{
	ScopedDebugInfo dbg_info(amx, "DCC_DeleteGuildRole", params, "dd");

	GuildId_t guildid = params[1];
	Guild_t const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	RoleId_t roleid = params[2];
	Role_t const &role = RoleManager::Get()->FindRole(roleid);
	if (!role)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid role id '{}'", roleid);
		return 0;
	}

	guild->DeleteRole(role);

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_BotPresenceStatus:DCC_GetBotPresenceStatus();
AMX_DECLARE_NATIVE(Native::DCC_GetBotPresenceStatus)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetBotPresenceStatus", params);
	return static_cast<cell>(ThisBot::Get()->GetPresenceStatus());
}

// native DCC_TriggerBotTypingIndicator(DCC_Channel:channel);
AMX_DECLARE_NATIVE(Native::DCC_TriggerBotTypingIndicator)
{
	ScopedDebugInfo dbg_info(amx, "DCC_TriggerBotTypingIndicator", params, "d");

	auto channelid = static_cast<ChannelId_t>(params[1]);
	auto const &channel = ChannelManager::Get()->FindChannel(channelid);
	if (!channel)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid channel id '{}'", channelid);
		return 0;
	}

	ThisBot::Get()->TriggerTypingIndicator(channel);
	return 1;
}

// native DCC_SetBotNickname(DCC_Guild:guild, const nickname[]);
AMX_DECLARE_NATIVE(Native::DCC_SetBotNickname)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetBotNickname", params, "ds");

	auto guildid = static_cast<GuildId_t>(params[1]);
	auto const &guild = GuildManager::Get()->FindGuild(guildid);
	if (!guild)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid guild id '{}'", guildid);
		return 0;
	}

	auto nickname = amx_GetCppString(amx, params[2]);
	if (!nickname.empty()) // if nickname is empty it gets resetted/removed
	{
		// see https://discordapp.com/developers/docs/resources/user#usernames-and-nicknames
		if (nickname.length() < 2 || nickname.length() > 32
			|| nickname == "discordtag" || nickname == "everyone" || nickname == "here"
			|| nickname.front() == '@' || nickname.front() == '#' || nickname.front() == ':'
			|| nickname.find("```") == 0)
		{
			Logger::Get()->LogNative(LogLevel::ERROR, "invalid nickname '{:s}'", nickname);
			return 0;
		}
	}

	ThisBot::Get()->SetNickname(guild, nickname);
	return 1;
}

// native DCC_CreatePrivateChannel(DCC_User:user, const callback[] = "",
//		const format[] = "", {Float, _}:...);
AMX_DECLARE_NATIVE(Native::DCC_CreatePrivateChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_CreatePrivateChannel", params, "dss");

	auto userid = static_cast<UserId_t>(params[1]);
	auto const &user = UserManager::Get()->FindUser(userid);
	if (!user)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "invalid user id '{}'", userid);
		return 0;
	}

	auto
		cb_name = amx_GetCppString(amx, params[2]),
		cb_format = amx_GetCppString(amx, params[3]);

	pawn_cb::Error cb_error;
	auto cb = pawn_cb::Callback::Prepare(
		amx, cb_name.c_str(), cb_format.c_str(), params, 4, cb_error);
	if (cb_error && cb_error.get() != pawn_cb::Error::Type::EMPTY_NAME)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "could not prepare callback");
		return 0;
	}

	ThisBot::Get()->CreatePrivateChannel(user, std::move(cb));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_Channel:DCC_GetCreatedPrivateChannel();
AMX_DECLARE_NATIVE(Native::DCC_GetCreatedPrivateChannel)
{
	ScopedDebugInfo dbg_info(amx, "DCC_GetCreatedPrivateChannel", params);
	return ThisBot::Get()->GetCreatedPrivateChannelId();
}

// native DCC_SetBotPresenceStatus(DCC_BotPresenceStatus:status);
AMX_DECLARE_NATIVE(Native::DCC_SetBotPresenceStatus)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetBotPresenceStatus", params, "d");

	bool ret_val = ThisBot::Get()->SetPresenceStatus(
		static_cast<ThisBot::PresenceStatus>(params[1]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}

// native DCC_SetBotActivity(const name[]);
AMX_DECLARE_NATIVE(Native::DCC_SetBotActivity)
{
	ScopedDebugInfo dbg_info(amx, "DCC_SetBotActivity", params, "s");

	ThisBot::Get()->SetActivity(amx_GetCppString(amx, params[1]));

	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '1'");
	return 1;
}

// native DCC_EscapeMarkdown(const src[], dest[], max_size = sizeof dest);
AMX_DECLARE_NATIVE(Native::DCC_EscapeMarkdown)
{
	ScopedDebugInfo dbg_info(amx, "DCC_EscapeMarkdown", params, "drs");

	auto const src = amx_GetCppString(amx, params[1]);
	std::string dest;
	dest.reserve(src.length());

	static const std::unordered_set<char> markdown_chars{
		'_', '*', '~', '`', '|'
	};

	bool skip = false;
	for (auto it = src.begin(); it != src.end(); ++it)
	{
		auto ch = *it;

		if (ch == '\\')
		{
			skip = true; // skip next escape
		}
		else if (markdown_chars.find(ch) != markdown_chars.end())
		{
			if (skip)
				skip = false;
			else
				dest.push_back('\\');
		}

		dest.push_back(ch);
	}

	if (amx_SetCppString(amx, params[2], dest, params[3]) != AMX_ERR_NONE)
	{
		Logger::Get()->LogNative(LogLevel::ERROR, "couldn't set destination string");
		return -1;
	}

	auto ret_val = static_cast<cell>(dest.length());
	Logger::Get()->LogNative(LogLevel::DEBUG, "return value: '{}'", ret_val);
	return ret_val;
}