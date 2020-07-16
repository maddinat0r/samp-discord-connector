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
	std::string author_id, channel_id;
	_valid =
		utils::TryGetJsonValue(data, m_Id, "id") &&
		utils::TryGetJsonValue(data, author_id, "author", "id") &&
		utils::TryGetJsonValue(data, channel_id, "channel_id") &&
		utils::TryGetJsonValue(data, m_Content, "content") &&
		utils::TryGetJsonValue(data, m_IsTts, "tts") &&
		utils::TryGetJsonValue(data, m_MentionsEveryone, "mention_everyone");

	if (!_valid)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"can't construct message object: invalid JSON: \"{}\"", data.dump());
		return;
	}

	Channel_t const &channel = ChannelManager::Get()->FindChannelById(channel_id);
	m_Channel = channel ? channel->GetPawnId() : INVALID_CHANNEL_ID;

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

				MessageManager::Get()->Delete(msg);
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
		Snowflake_t user_id, message_id;
		if (!utils::TryGetJsonValue(data, user_id, "user_id"))
			return;

		if (!utils::TryGetJsonValue(data, message_id, "message_id"))
			return;

		PawnDispatcher::Get()->Dispatch([data, user_id, message_id]() mutable
		{
			auto const& msg = MessageManager::Get()->FindById(message_id);
			auto const& user = UserManager::Get()->FindUserById(user_id);
			if (msg && user)
			{
				Snowflake_t emoji_id;
				std::string name;
				utils::TryGetJsonValue(data["emoji"], emoji_id, "id");
				if (!utils::TryGetJsonValue(data["emoji"], name, "name"))
					return;

				auto const id = EmojiManager::Get()->AddEmoji(emoji_id, name);
				// forward DCC_OnMessageReactionAdd(DCC_Message:message, DCC_User:reaction_user, DCC_Emoji:reaction_emoji);
				pawn_cb::Error error;
				pawn_cb::Callback::CallFirst(error, "DCC_OnMessageReactionAdd", msg->GetPawnId(), user->GetPawnId(), id);
				EmojiManager::Get()->DeleteEmoji(id);
			}
		});
	});
}

void Message::EditMessage(const std::string& msg, const EmbedId_t embedid)
{
	Channel_t const& channel = ChannelManager::Get()->FindChannel(GetChannel());
	if (!channel)
		return;

	json data = {
		{ "content", msg }
	};

	if (embedid != INVALID_EMBED_ID)
	{
		const auto& embed = EmbedManager::Get()->FindEmbed(embedid);
		if (embed)
		{
			data["embed"] =
			{
				{ "title", embed->GetTitle() },
				{ "description", embed->GetDescription() },
				{ "url", embed->GetUrl() },
				{ "timestamp", embed->GetTimestamp() },
				{ "color", embed->GetColor() },
				{ "footer", {
					{"text", embed->GetFooterText()},
					{"icon_url", embed->GetFooterIconUrl()},
				}},
				{"thumbnail", {
					{"url", embed->GetThumbnailUrl()}
				}},
				{"image", {
					{"url", embed->GetImageUrl()}
				}}
			};

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
	}

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}/messages/{:s}", channel->GetId(), GetId()), json_str);
}

MessageId_t MessageManager::Create(json const &data)
{
	MessageId_t id = 1;
	while (m_Messages.find(id) != m_Messages.end())
		++id;

	if (!m_Messages.emplace(id, Message_t(new Message(id, data))).first->second)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"can't create message: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(LogLevel::DEBUG, "created message with id '{}'", id);
	return id;
}

bool MessageManager::Delete(MessageId_t id)
{
	auto it = m_Messages.find(id);
	if (it == m_Messages.end())
		return false;

	m_Messages.erase(it);
	Logger::Get()->Log(LogLevel::DEBUG, "deleted message with id '{}'", id);
	return true;
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
