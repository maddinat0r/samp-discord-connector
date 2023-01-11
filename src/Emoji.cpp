#include "Emoji.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Logger.hpp"
#include "utils.hpp"


Emoji::Emoji(Snowflake_t const& id, std::string const& name) :
	_id(id),
	_name(name)
{
}

EmojiId_t EmojiManager::AddEmoji(Snowflake_t const & snowflake, std::string const & name)
{
	EmojiId_t id = 1;
	while (m_Emojis.find(id) != m_Emojis.end())
		++id;

	if (!m_Emojis.emplace(id, Emoji_t(new Emoji(snowflake, name))).first->second)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"can't create embed: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully created emoji with id '{}'", id);
	return id;
}

bool EmojiManager::DeleteEmoji(EmojiId_t id)
{
	if (m_Emojis.find(id) == m_Emojis.end())
	{
		Logger::Get()->Log(samplog_LogLevel::WARNING, "attempted to delete emoji with id '{}' but it does not exist", id);
		return false;
	}

	m_Emojis.erase(id);
	Logger::Get()->Log(samplog_LogLevel::INFO, "successfully deleted emoji with id '{}'", id);
	return true;
}

Emoji_t const& EmojiManager::FindEmoji(EmojiId_t id)
{
	static Emoji_t invalid_embed;
	auto it = m_Emojis.find(id);
	if (it == m_Emojis.end())
		return invalid_embed;
	return it->second;
}