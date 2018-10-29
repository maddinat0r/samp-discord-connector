#pragma once

#include "types.hpp"
#include "CSingleton.hpp"

#include <string>

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

	bool m_MentionsEveryone;

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
	bool MentionsEveryone() const
	{
		return m_MentionsEveryone;
	}

	bool IsValid() const
	{
		return _valid;
	}
	operator bool() const
	{
		return IsValid();
	}
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
	MessageId_t Create(json &data);
	bool Delete(MessageId_t id);

	Message_t const &Find(MessageId_t id);
};
