#include "CChannel.hpp"
#include "CNetwork.hpp"


CChannel::CChannel(json &data)
{
	m_Id = data["id"].get<std::string>();
	m_GuildId = data["guild_id"].get<std::string>();
	m_IsPrivate = data["is_private"].get<bool>();
	m_Name = data["name"].get<std::string>();
	json &topic = data["topic"];
	if (!topic.is_null())
		m_Topic = topic.get<std::string>();
}


void CChannelManager::Initialize()
{
	
}

void CChannelManager::AddChannel(json &data)
{
	unsigned int id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	m_Channels.emplace(id, std::make_unique<CChannel>(data));
}

Channel_t const &CChannelManager::FindChannel(unsigned int id)
{
	static Channel_t invalid_channel;
	auto it = m_Channels.find(id);
	if (it == m_Channels.end())
		return invalid_channel;
	return it->second;
}
