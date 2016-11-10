#pragma once

#include "types.hpp"

#include <string>

#include <json.hpp>

using json = nlohmann::json;


class CMessage
{
public:
	CMessage(json data);
	~CMessage() = default;

private:
	std::string
		m_Author,
		m_Content,
		m_ChannelId;

public:
	std::string const &GetAuthor() const
	{
		return m_Author;
	}
	std::string const &GetContent() const
	{
		return m_Content;
	}
	std::string const &GetChannelId() const
	{
		return m_ChannelId;
	}

};
