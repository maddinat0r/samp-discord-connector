#pragma once

#include "CSingleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>

#include <json.hpp>

#undef SendMessage // Windows at its finest


using json = nlohmann::json;


class Channel
{
	friend class ChannelManager;
public:
	enum class Type
	{
		GUILD_TEXT = 0,
		DM = 1,
		GUILD_VOICE = 2,
		GROUP_DM = 3,
		GUILD_CATEGORY = 4
	};

public:
	Channel(ChannelId_t pawn_id, json &data, GuildId_t guild_id);
	~Channel() = default;

private:
	const ChannelId_t m_PawnId;

	Snowflake_t m_Id;

	GuildId_t m_GuildId = 0;

	Type m_Type;

	std::string
		m_Name,
		m_Topic;

public:
	inline ChannelId_t GetPawnId() const
	{
		return m_PawnId;
	}
	inline Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	inline Type GetType() const
	{
		return m_Type;
	}
	inline GuildId_t GetGuildId() const
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

	void SendMessage(std::string &&msg);

};


class ChannelManager : public CSingleton<ChannelManager>
{
	friend class CSingleton<ChannelManager>;
private:
	ChannelManager() = default;
	~ChannelManager() = default;

private:
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };

	std::map<ChannelId_t, Channel_t> m_Channels; //PAWN channel-id to actual channel map

public:
	void Initialize();
	bool WaitForInitialization();

	Channel_t const &AddChannel(json &data, GuildId_t guild_id = 0);
	void UpdateChannel(json &data);
	void DeleteChannel(json &data);

	Channel_t const &FindChannel(ChannelId_t id);
	Channel_t const &FindChannelByName(std::string const &name);
	Channel_t const &FindChannelById(Snowflake_t const &sfid);
};
