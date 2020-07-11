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
	Embed(std::string const& title, std::string const& description, std::string const& url, std::string const & timestamp, int color, std::string const& footer_text, std::string const& footer_icon_url,
		std::string const& footer_proxy_icon_url, std::string const& thumbnail_url, std::string const& thumbnail_proxy_url, int thumbnail_height, int thumbnail_width);
	~Embed() = default;

	bool AddField(std::string const& name, std::string const& value, bool inline_ = false)
	{
		_fields.emplace_back(name, value, inline_);
		return true;
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
	inline std::string const& GetFooterIconProxyUrl()
	{
		return _footer_proxy_icon_url;
	}
	inline std::string const& GetThumbnailUrl()
	{
		return _thumbnail_url;
	}
	inline std::string const& GetThumbnailProxyUrl()
	{
		return _thumbnail_proxy_url;
	}
	inline std::string const& GetTimestamp()
	{
		return _timestamp;
	}

	inline int const GetColor()
	{
		return _color;
	}
	inline int const GetThumbnailHeight()
	{
		return _thumbnail_height;
	}
	inline int const GetThumbnailWidth()
	{
		return _thumbnail_width;
	}

	inline std::vector<EmbedField>& GetFields()
	{
		return _fields;
	}
private:
	std::string _title, _description, _url, _timestamp, _footer_text, _footer_icon_url, _footer_proxy_icon_url, _thumbnail_url, _thumbnail_proxy_url;
	int _color, _thumbnail_height, _thumbnail_width;
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
		std::string const & footer_proxy_icon_url, std::string const & thumbnail_url, std::string const & thumbnail_proxy_url, int thumbnail_height, int thumbnail_width);
	bool DeleteEmbed(EmbedId_t id);
	Embed_t const & FindEmbed(EmbedId_t id);
};
