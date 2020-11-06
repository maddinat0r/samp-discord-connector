#pragma once

#include "Singleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>
#include <vector>
#include <json.hpp>

using json = nlohmann::json;

class Emoji
{
public:
	Emoji(Snowflake_t const & id, std::string const & name);
	~Emoji() = default;

	inline Snowflake_t const& GetSnowflake()
	{
		return _id;
	}
	inline std::string const& GetName()
	{
		return _name;
	}
private:
	Snowflake_t _id;
	std::string _name;

};

class EmojiManager : public Singleton<EmojiManager>
{
	friend class Singleton<EmojiManager>;
private:
	EmojiManager() = default;
	~EmojiManager() = default;

private:
	std::map<EmojiId_t, Emoji_t> m_Emojis;
public:
	EmojiId_t AddEmoji(Snowflake_t const & snowflake, std::string const & name);
	bool DeleteEmoji(EmojiId_t id);
	Emoji_t const& FindEmoji(EmojiId_t id);
};