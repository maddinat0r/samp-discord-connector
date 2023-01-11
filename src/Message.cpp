#include "Message.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Role.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include "Emoji.hpp"
#include "Embed.hpp"

Message::Message(MessageId_t pawn_id, json const &data) : m_PawnId(pawn_id)
{
	std::string author_id, channel_id, guild_id;
	_valid =
		utils::TryGetJsonValue(data, m_Id, "id") &&
		utils::TryGetJsonValue(data, author_id, "author", "id") &&
		utils::TryGetJsonValue(data, channel_id, "channel_id") &&
		utils::TryGetJsonValue(data, m_Content, "content") &&
		utils::TryGetJsonValue(data, m_IsTts, "tts") &&
		utils::TryGetJsonValue(data, m_MentionsEveryone, "mention_everyone");

	if (!_valid)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't construct message object: invalid JSON: \"{}\"", data.dump());
		return;
	}

	Channel_t const &channel = ChannelManager::Get()->FindChannelById(channel_id);
	if (!channel && !utils::TryGetJsonValue(data, guild_id, "guild_id"))
	{
		ChannelId_t cid = ChannelManager::Get()->AddDMChannel(data);
		m_Channel = ChannelManager::Get()->FindChannel(cid)->GetPawnId();
	}
	else
	{
		m_Channel = channel ? channel->GetPawnId() : INVALID_CHANNEL_ID;
	}

	User_t const &user = UserManager::Get()->FindUserById(author_id);
	m_Author = user ? user->GetPawnId() : INVALID_USER_ID;

	if (utils::IsValidJson(data, "mentions", json::value_t::array))
	{
		for (auto &c : data["mentions"])
		{
			std::string mu_id;
			if (!utils::TryGetJsonValue(c, mu_id, "id"))
				continue;

			User_t const &mu = UserManager::Get()->FindUserById(mu_id);
			if (mu)
				m_UserMentions.push_back(mu->GetPawnId());
		}
	}

	if (utils::IsValidJson(data, "mention_roles", json::value_t::array))
	{
		for (auto &c : data["mention_roles"])
		{
			std::string mr_id;
			if (!utils::TryGetJsonValue(c, mr_id, "id"))
				continue;

			Role_t const &mr = RoleManager::Get()->FindRoleById(mr_id);
			if (mr)
				m_RoleMentions.push_back(mr->GetPawnId());
		}
	}
}

void Message::DeleteMessage()
{
	Channel_t const &channel = ChannelManager::Get()->FindChannel(GetChannel());
	if (!channel)
		return;

	Network::Get()->Http().Delete(fmt::format(
		"/channels/{:s}/messages/{:s}", channel->GetId(), GetId()));
}

void Message::AddReaction(Emoji_t const& emoji)
{
	Channel_t const& channel = ChannelManager::Get()->FindChannel(GetChannel());
	if (!channel)
		return;

	std::string emoji_str = emoji->GetName();

	if (emoji->GetSnowflake() != "")
	{
		emoji_str += ":" + emoji->GetSnowflake();
	}
	else
	{
		// This is a sort of hacky way thing, discord wants utf8 in hex: %F0%9F%99%82 (for hammer)
		// I did attempt to just send the unicode to it, but I had a 400 BAD REQUEST, this worked however.
		std::stringstream conversion;
		for (size_t i = 0; i < emoji_str.size(); i++)
		{
			conversion << "%" << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(emoji_str[i]));
		}
		emoji_str = conversion.str();
	}
	Network::Get()->Http().Put(fmt::format(
		"/channels/{:s}/messages/{:s}/reactions/{}/@me", channel->GetId(), GetId(), emoji_str
	));
}

bool Message::DeleteReaction(EmojiId_t const emojiid)
{
	Channel_t const& channel = ChannelManager::Get()->FindChannel(GetChannel());
	if (!channel)
		return false;

	std::string url = fmt::format("/channels/{:s}/messages/{:s}/reactions", channel->GetId(), GetId());

	if (emojiid != INVALID_EMOJI_ID)
	{
		const auto& emoji = EmojiManager::Get()->FindEmoji(emojiid);

		if (!emoji)
		{
			Logger::Get()->Log(samplog_LogLevel::ERROR, "invalid emoji id '{}'", emojiid);
			return false;
		}

		if (emoji->GetSnowflake() != "")
		{
			// custom emoji
			url += fmt::format("/{:s}:{:s}", emoji->GetName(), emoji->GetSnowflake());
		}
		else
		{
			// unicode emoji
			std::stringstream conversion;
			std::string emoji_name = emoji->GetName();
			for (size_t i = 0; i < emoji_name.size(); i++)
			{
				conversion << "%" << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(emoji_name[i]));
			}
			url += fmt::format("/{:s}", conversion.str());
		}
	}

	Network::Get()->Http().Delete(url);
	return true;
}

bool Message::EditMessage(const std::string& msg, const EmbedId_t embedid)
{
	Channel_t const& channel = ChannelManager::Get()->FindChannel(GetChannel());
	if (!channel)
		return false;

	json data = {
		{ "content", msg }
	};

	if (embedid != INVALID_EMBED_ID)
	{
		const auto& embed = EmbedManager::Get()->FindEmbed(embedid);
		if (!embed)
		{
			Logger::Get()->Log(samplog_LogLevel::ERROR, "invalid embed id {}", embedid);
			return false;
		}

		data["embed"] =
		{
			{ "title", embed->GetTitle() },
			{ "description", embed->GetDescription() },
			{ "url", embed->GetUrl() },
			{ "timestamp", embed->GetTimestamp() },
			{ "color", embed->GetColor() },
			{ "footer", {
				{"text", embed->GetFooterText()},
				//{"icon_url", embed->GetFooterIconUrl()},
			}},
			{"thumbnail", {
				//{"url", embed->GetThumbnailUrl()}
			}},
			{"image", {
				//{"url", embed->GetImageUrl()}
			}}
		};

		if (!embed->GetThumbnailUrl().empty())
		{
			data["embed"]["thumbnail"] += {"url", embed->GetThumbnailUrl()};
		}

		if (!embed->GetFooterIconUrl().empty())
		{
			data["embed"]["footer"] += {"icon_url", embed->GetFooterIconUrl()};
		}

		if (!embed->GetImageUrl().empty())
		{
			data["embed"]["image"] += {"url", embed->GetImageUrl()};
		}

		if (embed->GetFields().size())
		{
			json field_array = json::array();
			for (const auto& i : embed->GetFields())
			{
				field_array.push_back({
					{"name", i._name},
					{"value", i._value},
					{"inline", i._inline_}
					});
			}
			data["embed"]["fields"] = field_array;
		}
		EmbedManager::Get()->DeleteEmbed(embedid);
	}

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(samplog_LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}/messages/{:s}", channel->GetId(), GetId()), json_str);
	return true;
}

void MessageManager::Initialize()
{
	// PAWN callbacks
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, [](json const &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			MessageId_t msg = MessageManager::Get()->Create(data);
			if (msg != INVALID_MESSAGE_ID)
			{
				// forward DCC_OnMessageCreate(DCC_Message:message);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageCreate", msg);

				if (!MessageManager::Get()->Find(msg)->Persistent())
				{
					MessageManager::Get()->Delete(msg);
				}
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_DELETE, [](json const &data)
	{
		Snowflake_t sfid;
		if (!utils::TryGetJsonValue(data, sfid, "id"))
			return;

		PawnDispatcher::Get()->Dispatch([sfid]() mutable
		{
			auto const &msg = MessageManager::Get()->FindById(sfid);
			if (msg)
			{
				// forward DCC_OnMessageDelete(DCC_Message:message);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageDelete", msg->GetPawnId());

				MessageManager::Get()->Delete(msg->GetPawnId());
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_REACTION_ADD, [](json const& data)
	{
		Snowflake_t user_id, message_id, emoji_id;
		std::string name;
		if (!utils::TryGetJsonValue(data, user_id, "user_id"))
			return;

		if (!utils::TryGetJsonValue(data, message_id, "message_id"))
			return;
		if (!utils::TryGetJsonValue(data, name, "emoji", "name"))
			return;
		utils::TryGetJsonValue(data, emoji_id, "emoji", "id");

		PawnDispatcher::Get()->Dispatch([user_id, message_id, emoji_id, name]() mutable
		{
			auto const& msg = MessageManager::Get()->FindById(message_id);
			auto const& user = UserManager::Get()->FindUserById(user_id);
			if (msg && user)
			{
				auto const id = EmojiManager::Get()->AddEmoji(emoji_id, name);
				// forward DCC_OnMessageReactionAdd(DCC_Message:message, DCC_User:reaction_user, DCC_Emoji:emoji, DCC_MessageReactionType:reaction_type);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageReaction", msg->GetPawnId(), user->GetPawnId(), id, static_cast<int>(Message::ReactionType::REACTION_ADD));
				EmojiManager::Get()->DeleteEmoji(id);
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_REACTION_REMOVE, [](json const& data)
	{
		Snowflake_t user_id, message_id, emoji_id;
		std::string name;
		if (!utils::TryGetJsonValue(data, user_id, "user_id"))
			return;

		if (!utils::TryGetJsonValue(data, message_id, "message_id"))
			return;

		if (!utils::TryGetJsonValue(data, name, "emoji", "name"))
			return;
		utils::TryGetJsonValue(data, emoji_id, "emoji", "id");

		PawnDispatcher::Get()->Dispatch([data, user_id, message_id, emoji_id, name]() mutable
		{
			auto const& msg = MessageManager::Get()->FindById(message_id);
			auto const& user = UserManager::Get()->FindUserById(user_id);
			if (msg && user)
			{
				auto const id = EmojiManager::Get()->AddEmoji(emoji_id, name);
				// forward DCC_OnMessageReactionAdd(DCC_Message:message, DCC_User:reaction_user, DCC_Emoji:emoji, DCC_MessageReactionType:reaction_type);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageReaction", msg->GetPawnId(), user->GetPawnId(), id, static_cast<int>(Message::ReactionType::REACTION_REMOVE));
				EmojiManager::Get()->DeleteEmoji(id);
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_REACTION_REMOVE_ALL, [](json const& data)
	{
		Snowflake_t message_id;

		if (!utils::TryGetJsonValue(data, message_id, "message_id"))
			return;

		PawnDispatcher::Get()->Dispatch([message_id]() mutable
		{
			auto const& msg = MessageManager::Get()->FindById(message_id);
			if (msg)
			{
				// forward DCC_OnMessageReactionAdd(DCC_Message:message, DCC_User:reaction_user, DCC_Emoji:emoji, DCC_MessageReactionType:reaction_type);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageReaction", msg->GetPawnId(), INVALID_USER_ID, INVALID_EMOJI_ID, static_cast<int>(Message::ReactionType::REACTION_REMOVE_ALL));
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_REACTION_REMOVE_EMOJI, [](json const& data)
	{
		Snowflake_t  message_id, emoji_id;
		std::string name;

		if (!utils::TryGetJsonValue(data, message_id, "message_id"))
			return;
		if (!utils::TryGetJsonValue(data, name, "emoji", "name"))
			return;
		utils::TryGetJsonValue(data, emoji_id, "emoji", "id");

		PawnDispatcher::Get()->Dispatch([message_id, emoji_id, name]() mutable
		{
			auto const& msg = MessageManager::Get()->FindById(message_id);
			if (msg)
			{
				auto const id = EmojiManager::Get()->AddEmoji(emoji_id, name);
				// forward DCC_OnMessageReactionAdd(DCC_Message:message, DCC_User:reaction_user, DCC_Emoji:emoji, DCC_MessageReactionType:reaction_type);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageReaction", msg->GetPawnId(), INVALID_USER_ID, id, static_cast<int>(Message::ReactionType::REACTION_REMOVE_EMOJI));
				EmojiManager::Get()->DeleteEmoji(id);
			}
		});
	});
}

MessageId_t MessageManager::Create(json const &data)
{
	MessageId_t id = 1;
	while (m_Messages.find(id) != m_Messages.end())
		++id;

	if (!m_Messages.emplace(id, Message_t(new Message(id, data))).first->second)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't create message: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(samplog_LogLevel::DEBUG, "created message with id '{}'", id);
	return id;
}

bool MessageManager::Delete(MessageId_t id)
{
	auto it = m_Messages.find(id);
	if (it == m_Messages.end())
		return false;

	m_Messages.erase(it);
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "deleted message with id '{}'", id);
	return true;
}

void MessageManager::CreateFromSnowflake(Snowflake_t channel, Snowflake_t message, pawn_cb::Callback_t&& callback)
{
	Network::Get()->Http().Get(fmt::format("/channels/{:s}/messages/{:s}", channel, message),
		[this, callback](Http::Response r)
		{
			Logger::Get()->Log(samplog_LogLevel::DEBUG,
				"message fetch response: status {}; body: {}; add: {}",
				r.status, r.body, r.additional_data);
			if (r.status / 100 == 2) // success
			{
				const auto & message_id = Create(json::parse(r.body));
				if (callback)
				{
					PawnDispatcher::Get()->Dispatch([this, message_id, callback]() mutable
					{
						if (message_id)
						{
							SetCreatedMessageId(message_id);
							callback->Execute();
							SetCreatedMessageId(INVALID_MESSAGE_ID);
						}
					});
				}
				if (!Find(message_id)->Persistent())
				{
					Delete(message_id);
				}
			}
		}
	);
}

Message_t const &MessageManager::Find(MessageId_t id)
{
	static Message_t invalid_msg;
	auto it = m_Messages.find(id);
	if (it == m_Messages.end())
		return invalid_msg;
	return it->second;
}

Message_t const &MessageManager::FindById(Snowflake_t const &sfid)
{
	static Message_t invalid_msg;
	for (auto const &u : m_Messages)
	{
		auto const &msg = u.second;
		if (msg->GetId().compare(sfid) == 0)
			return msg;
	}
	return invalid_msg;
}
