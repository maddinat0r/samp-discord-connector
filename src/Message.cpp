#include "Message.hpp"
#include "User.hpp"
#include "utils.hpp"


Message::Message(json &data)
{
	std::string author_id;
	_valid =
		utils::TryGetJsonValue(data, author_id, "author", "id") &&
		utils::TryGetJsonValue(data, m_ChannelId, "channel_id") &&
		utils::TryGetJsonValue(data, m_Content, "content") &&
		utils::TryGetJsonValue(data, m_MentionsEveryone, "mention_everyone");

	User_t const &user = UserManager::Get()->FindUserById(author_id);
	m_Author = user ? user->GetPawnId() : 0;
}
