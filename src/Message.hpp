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
	std::string
		m_Author,
		m_Content;

	Snowflake_t m_ChannelId;

public:
	std::string const &GetAuthor() const
	{
		return m_Author;
	}
	std::string const &GetContent() const
	{
		return m_Content;
	}
	Snowflake_t const &GetChannelId() const
	{
		return m_ChannelId;
	}

};
