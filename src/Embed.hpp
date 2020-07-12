#pragma once

#include "Singleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>
#include <vector>
#include <json.hpp>

using json = nlohmann::json;

struct EmbedField
{
	EmbedField(std::string const& name, std::string const& value, bool inline_ = false) :
		_name(name),
		_value(value),
		_inline_(inline_)
	{
	}
	std::string const _name;
	std::string const _value;
	bool _inline_;
};

class Embed
{
public:
	Embed(std::string const& title, std::string const& description, std::string const& url, std::string const & timestamp, std::string const& footer_text, std::string const& footer_icon_url,
		std::string const& thumbnail_url, int color);
	~Embed() = default;

	bool AddField(std::string const& name, std::string const& value, bool inline_ = false)
	{
		_fields.emplace_back(name, value, inline_);
		return true;
	}

	void SetTitle(std::string const & title)
	{
		_title = title;
	}

	void SetDescription(std::string const & description)
	{
		_description = description;
	}

	void SetUrl(std::string const& url)
	{
		_url = url;
	}

	void SetFooterText(std::string const& footer_text)
	{
		_footer_text = footer_text;
	}

	void SetFooterIconUrl(std::string const& footer_icon_url)
	{
		_footer_icon_url = footer_icon_url;
	}

	void SetThumbnailUrl(std::string const& thumbnail_url)
	{
		_thumbnail_url = thumbnail_url;
	}

	void SetTimestamp(std::string const& timestamp)
	{
		_timestamp = timestamp;
	}

	void SetColor(int color)
	{
		_color = color;
	}

	inline std::string const& GetTitle()
	{
		return _title;
	}
	inline std::string const& GetDescription()
	{
		return _description;
	}
	inline std::string const& GetUrl()
	{
		return _url;
	}
	inline std::string const& GetFooterText()
	{
		return _footer_text;
	}
	inline std::string const& GetFooterIconUrl()
	{
		return _footer_icon_url;
	}
	inline std::string const& GetThumbnailUrl()
	{
		return _thumbnail_url;
	}
	inline std::string const& GetTimestamp()
	{
		return _timestamp;
	}

	inline int GetColor()
	{
		return _color;
	}
	inline std::vector<EmbedField>& GetFields()
	{
		return _fields;
	}
private:
	std::string _title, _description, _url, _timestamp, _footer_text, _footer_icon_url, _thumbnail_url;
	int _color;
	std::vector<EmbedField> _fields;
};


class EmbedManager : public Singleton<EmbedManager>
{
	friend class Singleton<EmbedManager>;
private:
	EmbedManager() = default;
	~EmbedManager() = default;

private:
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };
	std::map<EmbedId_t, Embed_t> m_Embeds;
public:
	EmbedId_t AddEmbed(std::string const & title, std::string const & description, std::string const & url, std::string const& timestamp, int color, std::string const & footer_text, std::string const & footer_icon_url,
		std::string const & thumbnail_url);
	bool DeleteEmbed(EmbedId_t id);
	Embed_t const & FindEmbed(EmbedId_t id);
};
