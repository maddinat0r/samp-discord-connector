#include "CChannel.hpp"
#include "CNetwork.hpp"

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
	CNetwork::Get()->HttpPost(fmt::format("/channels/{}/messages", GetId()), data.dump());
}


void CChannelManager::Initialize()
{
	CNetwork::Get()->WsRegisterEvent(CNetwork::WsEvent::CHANNEL_CREATE, [](json &data)
	{
		CChannelManager::Get()->AddChannel(data);
	});

	CNetwork::Get()->WsRegisterEvent(CNetwork::WsEvent::READY, [](json &data)
	{
		for (json &c : data["private_channels"])
			CChannelManager::Get()->AddChannel(c);
	});

	// GUILD_CREATE event seems to be always sent after READY event with all guilds the bot is in
	CNetwork::Get()->WsRegisterEvent(CNetwork::WsEvent::GUILD_CREATE, [](json &data)
	{
		for (json &c : data["channels"])
			CChannelManager::Get()->AddChannel(c);
	});
}

void CChannelManager::AddChannel(json &data)
{
	json &type = data["type"];
	if (!type.is_null() && type.get<std::string>() != "text")
		return; // we're only interested in text channels

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	m_Channels.emplace(id, std::make_unique<CChannel>(id, data));
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
