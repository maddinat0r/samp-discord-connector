#pragma once

#include "types.hpp"
#include "Singleton.hpp"

#include <string>
#include <vector>

#include <json.hpp>

using json = nlohmann::json;


class Message
{
	friend class MessageManager;
private:
	Message() : _valid(false)
	{ }
	Message(MessageId_t pawn_id, json const &data);
public:
	~Message() = default;

private:
	Snowflake_t m_Id;
	MessageId_t m_PawnId = INVALID_MESSAGE_ID;

	ChannelId_t m_Channel = INVALID_CHANNEL_ID;
	UserId_t m_Author = INVALID_USER_ID;

	std::string m_Content;
	bool m_IsTts = false;

	bool m_MentionsEveryone = false;
	std::vector<UserId_t> m_UserMentions;
	std::vector<RoleId_t> m_RoleMentions;

	bool _valid;

public:
	Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	MessageId_t GetPawnId() const
	{
		return m_PawnId;
	}
	ChannelId_t const &GetChannel() const
	{
		return m_Channel;
	}
	UserId_t const &GetAuthor() const
	{
		return m_Author;
	}
	std::string const &GetContent() const
	{
		return m_Content;
	}
	bool IsTts() const
	{
		return m_IsTts;
	}
	bool MentionsEveryone() const
	{
		return m_MentionsEveryone;
	}
	decltype(m_UserMentions) const &GetUserMentions() const
	{
		return m_UserMentions;
	}
	decltype(m_RoleMentions) const &GetRoleMentions() const
	{
		return m_RoleMentions;
	}

	bool IsValid() const
	{
		return _valid;
	}
	operator bool() const
	{
		return IsValid();
	}

	void DeleteMessage();
	void AddReaction(Emoji_t const& emoji);
	void EditMessage(const std::string& msg);
	void EditEmbeddedMessage(const Embed_t& embed, const std::string& msg);
};


class MessageManager : public Singleton<MessageManager>
{
	friend class Singleton<MessageManager>;
private:
	MessageManager() = default;
	~MessageManager() = default;

private:
	std::map<MessageId_t, Message_t> m_Messages; //PAWN message-id to actual channel map

	MessageId_t m_CreatedMessageId = INVALID_MESSAGE_ID;

public:
	void Initialize();

	MessageId_t GetCreatedMessageId() const
	{
		return m_CreatedMessageId;
	}
	void SetCreatedMessageId(MessageId_t id)
	{
		m_CreatedMessageId = id;
	}

	MessageId_t Create(json const &data);
	bool Delete(MessageId_t id);

	Message_t const &Find(MessageId_t id);
	Message_t const &FindById(Snowflake_t const &sfid);
};
