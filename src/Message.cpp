#include "Message.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Role.hpp"
#include "Network.hpp"
#include "PawnCallback.hpp"
#include "PawnDispatcher.hpp"
#include "CLog.hpp"
#include "utils.hpp"


Message::Message(MessageId_t pawn_id, json &data) : m_PawnId(pawn_id)
{
	std::string author_id, channel_id;
	_valid =
		utils::TryGetJsonValue(data, author_id, "author", "id") &&
		utils::TryGetJsonValue(data, channel_id, "channel_id") &&
		utils::TryGetJsonValue(data, m_Content, "content") &&
		utils::TryGetJsonValue(data, m_IsTts, "tts") &&
		utils::TryGetJsonValue(data, m_MentionsEveryone, "mention_everyone");

	if (!_valid)
	{
		CLog::Get()->Log(LogLevel::ERROR,
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

void MessageManager::Initialize()
{
	// PAWN callbacks
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			MessageId_t msg = MessageManager::Get()->Create(data);
			if (msg != INVALID_MESSAGE_ID)
			{
				// forward DCC_OnMessageCreate(DCC_Message:message);
				PawnCallbackManager::Get()->Call("DCC_OnMessageCreate", msg);

				MessageManager::Get()->Delete(msg);
			}
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_DELETE, [](json &data)
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
				PawnCallbackManager::Get()->Call("DCC_OnMessageDelete", msg->GetPawnId());

				MessageManager::Get()->Delete(msg->GetPawnId());
			}
		});
	});
}

MessageId_t MessageManager::Create(json &data)
{
	MessageId_t id = 1;
	while (m_Messages.find(id) != m_Messages.end())
		++id;

	if (!m_Messages.emplace(id, Message_t(new Message(id, data))).first->second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create message: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	CLog::Get()->Log(LogLevel::INFO, "successfully created message with id '{}'", id);
	return id;
}

bool MessageManager::Delete(MessageId_t id)
{
	auto it = m_Messages.find(id);
	if (it == m_Messages.end())
		return false;

	m_Messages.erase(it);
	CLog::Get()->Log(LogLevel::DEBUG, "deleted message with id '{}'", id);
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
