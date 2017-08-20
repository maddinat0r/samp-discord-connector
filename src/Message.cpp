#include "Message.hpp"
#include "User.hpp"


Message::Message(json &data)
{
	User_t const &user = UserManager::Get()->FindUserById(data["author"]["id"].get<std::string>());
	m_ChannelId = data["channel_id"].get<std::string>();
	m_Author = user ? user->GetPawnId() : 0;
	m_Content = data["content"].get<std::string>();
	m_MentionsEveryone = data["mention_everyone"].get<bool>();
}
