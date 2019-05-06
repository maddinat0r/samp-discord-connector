#pragma once

#include "Singleton.hpp"
#include "types.hpp"
#include "Callback.hpp"

#include <string>
#include <atomic>

#include <json.hpp>

#undef SendMessage // Windows at its finest


using json = nlohmann::json;


class Channel
{
	friend class ChannelManager;
public:
	enum class Type : unsigned int
	{
		GUILD_TEXT = 0,
		DM = 1,
		GUILD_VOICE = 2,
		GROUP_DM = 3,
		GUILD_CATEGORY = 4
	};

public:
	Channel(ChannelId_t pawn_id, json const &data, GuildId_t guild_id);
	~Channel() = default;

private:
	const ChannelId_t m_PawnId;

	Snowflake_t m_Id;

	GuildId_t m_GuildId = 0;

	Type m_Type;

	std::string
		m_Name,
		m_Topic;

	int m_Position = -1;
	bool m_IsNsfw = false;

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
	inline int GetPosition() const
	{
		return m_Position;
	}
	inline bool IsNsfw() const
	{
		return m_IsNsfw;
	}

	void SendMessage(std::string &&msg);
	void SetChannelName(std::string const &name);
	void SetChannelTopic(std::string const &topic);
	void SetChannelPosition(int const position);
	void SetChannelNsfw(bool const is_nsfw);
	void SetChannelParentCategory(Channel_t const &parent);
	void DeleteChannel();
};


class ChannelManager : public Singleton<ChannelManager>
{
	friend class Singleton<ChannelManager>;
private:
	ChannelManager() = default;
	~ChannelManager() = default;

private:
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };

	std::map<ChannelId_t, Channel_t> m_Channels; //PAWN channel-id to actual channel map
	ChannelId_t m_CreatedChannelId = INVALID_CHANNEL_ID;

public:
	void Initialize();
	bool IsInitialized();

	bool CreateGuildChannel(Guild_t const &guild, 
		std::string const &name, Channel::Type type, pawn_cb::Callback_t &&callback);
	ChannelId_t GetCreatedGuildChannelId() const
	{
		return m_CreatedChannelId;
	}

	ChannelId_t AddChannel(json const &data, GuildId_t guild_id = 0);
	void UpdateChannel(json const &data);
	void DeleteChannel(json const &data);

	Channel_t const &FindChannel(ChannelId_t id);
	Channel_t const &FindChannelByName(std::string const &name);
	Channel_t const &FindChannelById(Snowflake_t const &sfid);
};
