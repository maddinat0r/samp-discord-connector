#pragma once

#include "types.hpp"

#include <string>

#include <json.hpp>

using json = nlohmann::json;


class Message
{
public:
	Message(json &data);
	~Message() = default;

private:
	Snowflake_t
		m_Id,
		m_ChannelId;

	UserId_t m_Author;
	std::string m_Content;

	bool m_MentionsEveryone;

	bool _valid;

public:
	Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	Snowflake_t const &GetChannelId() const
	{
		return m_ChannelId;
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
