#include "Channel.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "CLog.hpp"
#include "Guild.hpp"
#include "utils.hpp"

#include "fmt/format.h"


#undef SendMessage // Windows at its finest


Channel::Channel(ChannelId_t pawn_id, json &data, GuildId_t guild_id) :
	m_PawnId(pawn_id)
{
	std::underlying_type<Type>::type type;
	if (!utils::TryGetJsonValue(data, type, "type")
		|| !utils::TryGetJsonValue(data, m_Id, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"type\" and \"id\" in \"{}\"", data.dump());
		return;
	}

	m_Type = static_cast<Type>(type);

	if (guild_id != 0)
	{
		m_GuildId = guild_id;
	}
	else
	{
		std::string guild_id;
		if (utils::TryGetJsonValue(data, guild_id, "guild_id"))
		{
			Guild_t const &guild = GuildManager::Get()->FindGuildById(guild_id);
			m_GuildId = guild->GetPawnId();
			guild->AddChannel(pawn_id);
		}
		else
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"guild_id\" in \"{}\"", data.dump());
		}
	}

	utils::TryGetJsonValue(data, m_Name, "name");
	utils::TryGetJsonValue(data, m_Topic, "topic");
	utils::TryGetJsonValue(data, m_Position, "position");
	utils::TryGetJsonValue(data, m_IsNsfw, "nsfw");
}

void Channel::SendMessage(std::string &&msg)
{
	json data = {
		{ "content", std::move(msg) }
	};
	
	std::string json_str;
	if(!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Post(fmt::format("/channels/{:s}/messages", GetId()), json_str);
}

void Channel::SetChannelName(std::string const &name)
{
	json data = {
		{ "name", name }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelTopic(std::string const &topic)
{
	json data = {
		{ "topic", topic }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelPosition(int const position)
{
	json data = {
		{ "position", position }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelNsfw(bool const is_nsfw)
{
	json data = {
		{ "nsfw", is_nsfw }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelParentCategory(Channel_t const &parent)
{
	if (parent->GetType() != Type::GUILD_CATEGORY)
		return;

	json data = {
		{ "parent_id", parent->GetId() }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		CLog::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::DeleteChannel()
{
	Network::Get()->Http().Delete(fmt::format("/channels/{:s}", GetId()));
}


void ChannelManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_CREATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(data);
			if (channel_id == INVALID_CHANNEL_ID)
				return;

			// forward DCC_OnChannelCreate(DCC_Channel:channel);
			pawn_cb::Error error;
			pawn_cb::Callback::CallFirst(error, "DCC_OnChannelCreate", channel_id);
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_UPDATE, [](json &data)
	{
		ChannelManager::Get()->UpdateChannel(data);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_DELETE, [](json &data)
	{
		ChannelManager::Get()->DeleteChannel(data);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		static const char *PRIVATE_CHANNEL_KEY = "private_channels";
		if (utils::IsValidJson(data, PRIVATE_CHANNEL_KEY, json::value_t::array))
		{
			for (json &c : data.at(PRIVATE_CHANNEL_KEY))
				AddChannel(c);
		}
		else
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"{}\" in \"{}\"", PRIVATE_CHANNEL_KEY, data.dump());
		}
		m_Initialized++;
	});
}

bool ChannelManager::IsInitialized()
{
	return m_Initialized == m_InitValue;
}

ChannelId_t ChannelManager::AddChannel(json &data, GuildId_t guild_id/* = 0*/)
{
	std::underlying_type<Channel::Type>::type type_u;
	if (!utils::TryGetJsonValue(data, type_u, "type"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"type\" in \"{}\"", data.dump());
		return INVALID_CHANNEL_ID;
	}

	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_CHANNEL_ID;
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (channel)
		return INVALID_CHANNEL_ID; // channel already exists

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	if (!m_Channels.emplace(id, Channel_t(new Channel(id, data, guild_id))).second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create channel: duplicate key '{}'", id);
		return INVALID_CHANNEL_ID;
	}

	CLog::Get()->Log(LogLevel::INFO, "successfully created channel with id '{}'", id);
	return id;
}

void ChannelManager::UpdateChannel(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (!channel)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't update channel: channel id \"{}\" not cached", sfid);
		return;
	}

	std::string name, topic;
	int position;
	bool is_nsfw;
	bool 
		update_name = utils::TryGetJsonValue(data, name, "name"),
		update_topic = utils::TryGetJsonValue(data, topic, "topic"),
		update_position = utils::TryGetJsonValue(data, position, "position"),
		update_nsfw = utils::TryGetJsonValue(data, is_nsfw, "nsfw");

	PawnDispatcher::Get()->Dispatch([=, &channel]()
	{
		if (update_name && !name.empty())
			channel->m_Name = name;
		if (update_topic && !topic.empty())
			channel->m_Topic = topic;
		if (update_position)
			channel->m_Position = position;
		if (update_nsfw)
			channel->m_IsNsfw = is_nsfw;

		// forward DCC_OnChannelUpdate(DCC_Channel:channel);
		pawn_cb::Error error;
		pawn_cb::Callback::CallFirst(error, "DCC_OnChannelUpdate", channel->GetPawnId());
	});
}

void ChannelManager::DeleteChannel(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (!channel)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't delete channel: channel id \"{}\" not cached", sfid);
		return;
	}

	PawnDispatcher::Get()->Dispatch([this, &channel]()
	{
		// forward DCC_OnChannelDelete(DCC_Channel:channel);
		pawn_cb::Error error;
		pawn_cb::Callback::CallFirst(error, "DCC_OnChannelDelete", channel->GetPawnId());

		Guild_t const &guild = GuildManager::Get()->FindGuild(channel->GetGuildId());
		if (guild)
			guild->RemoveChannel(channel->GetPawnId());

		m_Channels.erase(channel->GetPawnId());
	});

}

Channel_t const &ChannelManager::FindChannel(ChannelId_t id)
{
	static Channel_t invalid_channel;
	auto it = m_Channels.find(id);
	if (it == m_Channels.end())
		return invalid_channel;
	return it->second;
}

Channel_t const &ChannelManager::FindChannelByName(std::string const &name)
{
	static Channel_t invalid_channel;
	for (auto const &c : m_Channels)
	{
		Channel_t const &channel = c.second;
		if (channel->GetName().compare(name) == 0)
			return channel;
	}
	return invalid_channel;
}

Channel_t const &ChannelManager::FindChannelById(Snowflake_t const &sfid)
{
	static Channel_t invalid_channel;
	for (auto const &c : m_Channels)
	{
		Channel_t const &channel = c.second;
		if (channel->GetId().compare(sfid) == 0)
			return channel;
	}
	return invalid_channel;
}
