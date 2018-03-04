#include "Channel.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "CLog.hpp"
#include "PawnCallback.hpp"
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
		return; // TODO error msg: invalid json
	}

	m_Type = static_cast<Type>(type);

	if (m_Type == Type::GUILD_TEXT) // is a guild channel
	{
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
				// TODO: error message (is a guild channel, but no guild id was given)
			}
		}

		m_Name.clear();
		utils::TryGetJsonValue(data, m_Name, "name");
		utils::TryGetJsonValue(data, m_Topic, "topic");
	}
}

void Channel::SendMessage(std::string &&msg)
{
	json data = {
		{ "content", std::move(msg) }
	};

	try
	{
		Network::Get()->Http().Post(fmt::format("/channels/{}/messages", GetId()), data.dump());
	}
	catch (const json::type_error &e)
	{
		if (e.id == 316)
		{
			// TODO: error msg: invalid UTF-8 string (e.what())
		}
		else
		{
			// TODO: error msg: error while serializing json: e.what()
		}
	}
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
			PawnCallbackManager::Get()->Call("DCC_OnChannelCreate", channel_id);
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
			// TODO: error msg: invalid json
		}
		m_Initialized++;
	});

	// PAWN callbacks
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, 
		[this](json &data)
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

bool ChannelManager::WaitForInitialization()
{
	unsigned int const
		SLEEP_TIME_MS = 20,
		TIMEOUT_TIME_MS = 20 * 1000;
	unsigned int waited_time = 0;
	while (m_Initialized != m_InitValue)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
		waited_time += SLEEP_TIME_MS;
		if (waited_time > TIMEOUT_TIME_MS)
			return false;
	}
	return true;
}

ChannelId_t ChannelManager::AddChannel(json &data, GuildId_t guild_id/* = 0*/)
{
	std::underlying_type<Channel::Type>::type type_u;
	if (!utils::TryGetJsonValue(data, type_u, "type"))
	{
		return INVALID_CHANNEL_ID; // TODO error msg: invalid json
	}

	auto ch_type = static_cast<Channel::Type>(type_u);
	if (ch_type != Channel::Type::GUILD_TEXT
		&& ch_type != Channel::Type::DM
		&& ch_type != Channel::Type::GROUP_DM)
	{
		return INVALID_CHANNEL_ID; // we're only interested in text channels
	}

	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		return INVALID_CHANNEL_ID; // TODO error msg: invalid json
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (channel)
		return INVALID_CHANNEL_ID; // channel already exists

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	if (!m_Channels.emplace(id, Channel_t(new Channel(id, data, guild_id))).second)
		return INVALID_CHANNEL_ID; // TODO: error msg: duplicate key

	return id;
}

void ChannelManager::UpdateChannel(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		return; // TODO error msg: invalid json
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (!channel)
	{
		// TODO: error msg
		return;
	}

	std::string name, topic;
	utils::TryGetJsonValue(data, name, "name");
	utils::TryGetJsonValue(data, topic, "topic");

	PawnDispatcher::Get()->Dispatch([name, topic, &channel]()
	{
		if (!name.empty())
			channel->m_Name = name;
		if (!topic.empty())
			channel->m_Topic = topic;

		// forward DCC_OnChannelUpdate(DCC_Channel:channel);
		PawnCallbackManager::Get()->Call("DCC_OnChannelUpdate", channel->GetPawnId());
	});
}

void ChannelManager::DeleteChannel(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		return; // TODO error msg: invalid json
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (!channel)
	{
		// TODO: error msg
		return;
	}

	PawnDispatcher::Get()->Dispatch([this, &channel]()
	{
		// forward DCC_OnChannelDelete(DCC_Channel:channel);
		PawnCallbackManager::Get()->Call("DCC_OnChannelDelete", channel->GetPawnId());

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
