#include "CMessage.hpp"


CMessage::CMessage(json &data)
{ 
	m_Author = data["author"]["username"].get<std::string>();
	m_Content = data["content"].get<std::string>();
	m_ChannelId = data["channel_id"].get<std::string>();
}
