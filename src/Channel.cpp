#include "Channel.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "CLog.hpp"
#include "CCallback.hpp"
#include "Guild.hpp"

#include "fmt/format.h"


#undef SendMessage // Windows at its finest


Channel::Channel(ChannelId_t pawn_id, json &data, GuildId_t guild_id) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	m_Type = static_cast<Type>(data["type"].get<int>());
	if (m_Type == Type::GUILD_TEXT) // is a guild channel
	{
		if (guild_id != 0)
		{
			m_GuildId = guild_id;
		}
		else
		{
			auto it = data.find("guild_id");
			if (it != data.end() && !it->is_null())
			{
				Guild_t const &guild = GuildManager::Get()->FindGuildById(it->get<std::string>());
				m_GuildId = guild->GetPawnId();
				guild->AddChannel(pawn_id);
			}
			else
			{
				// TODO: error message (is a guild channel, but no guild id was given)
			}
		}
		m_Name = data["name"].get<std::string>();
		json &topic = data["topic"];
		if (!topic.is_null())
			m_Topic = topic.get<std::string>();
	}
}

void Channel::SendMessage(std::string &&msg)
{
	json data = {
		{ "content", std::move(msg) }
	};
	Network::Get()->Http().Post(fmt::format("/channels/{}/messages", GetId()), data.dump());
}


void ChannelManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_CREATE, [](json &data)
	{
		ChannelManager::Get()->AddChannel(data);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		for (json &c : data["private_channels"])
			AddChannel(c);
		m_Initialized++;
	});

	// GUILD_CREATE event seems to be always sent after READY event with all guilds the bot is in
	//Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_CREATE, [this](json &data)
	//{
	//	for (json &c : data["channels"])
	//	{
	//		// manually add guild_id to channel data
	//		c["guild_id"] = data["id"];
	//		AddChannel(c);
	//	}
	//	m_Initialized++;
	//});

	// PAWN callbacks
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, [this](json &data)
	{
		Message msg(data);
		Channel_t const &channel = FindChannelById(msg.GetChannelId());
		if (channel)
		{
			PawnDispatcher::Get()->Dispatch([this, msg, &channel]()
			{
				// forward DCC_OnChannelMessage(DCC_Channel:channel, DCC_User:author, const message[]);
				PawnCallbackManager::Get()->Call("DCC_OnChannelMessage",
					channel->GetPawnId(), msg.GetAuthor(), msg.GetContent());
			});
		}
	});
}

void ChannelManager::WaitForInitialization()
{
	while (m_Initialized != m_InitValue)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

Channel_t const &ChannelManager::AddChannel(json &data, GuildId_t guild_id/* = 0*/)
{
	static Channel_t invalid_channel;
	json &type = data["type"];
	if (!type.is_null())
	{
		Channel::Type ch_type = static_cast<Channel::Type>(type.get<int>());
		if (ch_type != Channel::Type::GUILD_TEXT
			&& ch_type != Channel::Type::DM
			&& ch_type != Channel::Type::GROUP_DM)
		{
			return invalid_channel; // we're only interested in text channels
		}
	}

	Snowflake_t sfid = data["id"].get<std::string>();
	Channel_t const &channel = FindChannelById(sfid);
	if (channel)
		return channel; // channel already exists

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	return m_Channels.emplace(id, Channel_t(new Channel(id, data, guild_id))).first->second;
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
