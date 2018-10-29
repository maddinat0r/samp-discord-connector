#pragma once

#include "types.hpp"
#include "CSingleton.hpp"

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
	Message(json &data);
public:
	~Message() = default;

private:
	Snowflake_t m_Id;

	ChannelId_t m_Channel = INVALID_CHANNEL_ID;
	UserId_t m_Author = INVALID_USER_ID;

	std::string m_Content;
	bool m_IsTts;

	bool m_MentionsEveryone;
	std::vector<UserId_t> m_UserMentions;
	std::vector<RoleId_t> m_RoleMentions;

	bool _valid;

public:
	Snowflake_t const &GetId() const
	{
		return m_Id;
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
	auto const &GetUserMentions() const
	{
		return m_UserMentions;
	}
	auto const &GetRoleMentions() const
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
};


class MessageManager : public CSingleton<MessageManager>
{
	friend class CSingleton<MessageManager>;
private:
	MessageManager() = default;
	~MessageManager() = default;

private:
	std::map<MessageId_t, Message_t> m_Messages; //PAWN message-id to actual channel map

public:
	void Initialize();

	MessageId_t Create(json &data);
	bool Delete(MessageId_t id);

	Message_t const &Find(MessageId_t id);
};
