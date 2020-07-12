#include "Embed.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "Logger.hpp"
#include "utils.hpp"


Embed::Embed(std::string const& title, std::string const& description, std::string const& url, std::string const& timestamp, std::string const& footer_text, std::string const& footer_icon_url,
	std::string const& thumbnail_url, int color) :
	_title(title),
	_description(description),
	_url(url),
	_timestamp(timestamp),
	_footer_text(footer_text),
	_footer_icon_url(footer_icon_url),
	_thumbnail_url(thumbnail_url),
	_color(color),
	_fields({})
{
}

EmbedId_t EmbedManager::AddEmbed(std::string const& title, std::string const& description, std::string const& url, std::string const& timestamp, int color, std::string const& footer_text, std::string const& footer_icon_url,
	std::string const& thumbnail_url)
{
	UserId_t id = 1;
	while (m_Embeds.find(id) != m_Embeds.end())
		++id;

	if (!m_Embeds.emplace(id, Embed_t(new Embed(title, description, url, timestamp, footer_text, footer_icon_url, thumbnail_url,
		color))).first->second)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"can't create embed: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	Logger::Get()->Log(LogLevel::INFO, "successfully created embed with id '{}'", id);
	return id;
}

bool EmbedManager::DeleteEmbed(EmbedId_t id)
{
	if (m_Embeds.find(id) == m_Embeds.end())
	{
		Logger::Get()->Log(LogLevel::WARNING, "attempted to delete embed with id '{}' but it does not exist", id);
		return false;
	}

	m_Embeds.erase(id);
	Logger::Get()->Log(LogLevel::INFO, "successfully deleted embed with id '{}'", id);
	return true;
}

Embed_t const & EmbedManager::FindEmbed(EmbedId_t id)
{
	static Embed_t invalid_embed;
	auto it = m_Embeds.find(id);
	if (it == m_Embeds.end())
		return invalid_embed;
	return it->second;
}