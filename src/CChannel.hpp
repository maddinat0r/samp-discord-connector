#pragma once

#include "CSingleton.hpp"
#include "types.hpp"

#include <string>

#include <json.hpp>

using json = nlohmann::json;


class CChannel
{
public:
	CChannel(json &data);
	~CChannel() = default;

private:
	Snowflake_t
		m_Id,
		m_GuildId;

	bool m_IsPrivate;

	std::string
		m_Name,
		m_Topic;

public:
	inline Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	inline Snowflake_t const &GetGuildId() const
	{
		return m_GuildId;
	}
	inline std::string const &GetName() const
	{
		return m_Name;
	}
	inline std::string const &GetTopic() const
	{
		return m_Topic;
	}

};


class CChannelManager : public CSingleton<CChannelManager>
{
	friend class CSingleton<CChannelManager>;
private:
	CChannelManager() = default;
	~CChannelManager() = default;

private:
	std::map<unsigned int, Channel_t> m_Channels; //PAWN channel-id to actual channel map

public:
	void Initialize();

	void AddChannel(json &data);
	Channel_t const &FindChannel(unsigned int id);
};
