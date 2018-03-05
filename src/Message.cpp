#include "Message.hpp"
#include "User.hpp"
#include "CLog.hpp"
#include "utils.hpp"


Message::Message(json &data)
{
	std::string author_id;
	_valid =
		utils::TryGetJsonValue(data, author_id, "author", "id") &&
		utils::TryGetJsonValue(data, m_ChannelId, "channel_id") &&
		utils::TryGetJsonValue(data, m_Content, "content") &&
		utils::TryGetJsonValue(data, m_MentionsEveryone, "mention_everyone");

	if (!_valid)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't construct message object: invalid JSON: \"{}\"", data.dump());
		return;
	}

	User_t const &user = UserManager::Get()->FindUserById(author_id);
	m_Author = user ? user->GetPawnId() : INVALID_USER_ID;
}
