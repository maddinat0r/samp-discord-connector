#include "CChannel.hpp"
#include "CMessage.hpp"
#include "Network.hpp"
#include "CDispatcher.hpp"
#include "CLog.hpp"

#include "fmt/format.h"


CChannel::CChannel(ChannelId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	m_IsPrivate = data["is_private"].get<bool>();
	if (!m_IsPrivate) // is a guild channel
	{
		m_GuildId = data["guild_id"].get<std::string>();
		m_Name = data["name"].get<std::string>();
		json &topic = data["topic"];
		if (!topic.is_null())
			m_Topic = topic.get<std::string>();
	}
}

void CChannel::SendMessage(std::string &&msg)
{
	json data = {
		{ "content", std::move(msg) }
	};
	Network::Get()->Http().Post(fmt::format("/channels/{}/messages", GetId()), data.dump());
}


void CChannelManager::Initialize(AMX *amx)
{
	if (m_Initialized == m_InitValue)
		return; // we've already been initialized

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_CREATE, [](json &data)
	{
		CChannelManager::Get()->AddChannel(data);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		for (json &c : data["private_channels"])
			AddChannel(c);
		m_Initialized++;
	});

	// GUILD_CREATE event seems to be always sent after READY event with all guilds the bot is in
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_CREATE, [this](json &data)
	{
		for (json &c : data["channels"])
		{
			// manually add guild_id to channel data
			c["guild_id"] = data["id"];
			AddChannel(c);
		}
		m_Initialized++;
	});

	// PAWN callbacks
	CError<CCallback> cb_error;

	// forward DCC_OnChannelMessage(DCC_Channel:channel, const author[], const message[]);
	m_ChannelMessage = CCallback::Create(cb_error, amx, "DCC_OnChannelMessage");
	if (cb_error != CCallback::Error::NOT_FOUND)
	{
		if (cb_error)
		{
			CLog::Get()->Log(LogLevel::ERROR, "{} error: {}",
				cb_error.module(), cb_error.msg());
		}
		else
		{
			Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, [this](json &data)
			{
				CMessage msg(data);
				Channel_t const &channel = FindChannelById(msg.GetChannelId());
				if (channel)
				{
					CDispatcher::Get()->Dispatch([this, msg, &channel]()
					{
						m_ChannelMessage->Execute(channel->GetPawnId(), msg.GetAuthor(), msg.GetContent());
					});
				}
			});
		}
	}
}

void CChannelManager::WaitForInitialization()
{
	while (m_Initialized != m_InitValue)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void CChannelManager::AddChannel(json &data)
{
	json &type = data["type"];
	if (!type.is_null() && type.get<std::string>() != "text")
		return; // we're only interested in text channels

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	m_Channels.emplace(id, Channel_t(new CChannel(id, data)));
}

Channel_t const &CChannelManager::FindChannel(ChannelId_t id)
{
	static Channel_t invalid_channel;
	auto it = m_Channels.find(id);
	if (it == m_Channels.end())
		return invalid_channel;
	return it->second;
}

Channel_t const &CChannelManager::FindChannelByName(std::string const &name)
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

Channel_t const &CChannelManager::FindChannelById(Snowflake_t const &sfid)
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
