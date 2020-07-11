#include "Channel.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Logger.hpp"
#include "Guild.hpp"
#include "utils.hpp"
#include "Embed.hpp"

#include "fmt/format.h"


#undef SendMessage // Windows at its finest


Channel::Channel(ChannelId_t pawn_id, json const &data, GuildId_t guild_id) :
	m_PawnId(pawn_id)
{
	std::underlying_type<Type>::type type;
	if (!utils::TryGetJsonValue(data, type, "type")
		|| !utils::TryGetJsonValue(data, m_Id, "id"))
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"type\" and \"id\" in \"{}\"", data.dump());
		return;
	}

	m_Type = static_cast<Type>(type);
	if (m_Type == Type::GUILD_TEXT || m_Type == Type::GUILD_VOICE || m_Type == Type::GUILD_CATEGORY)
	{
		if (guild_id != 0)
		{
			m_GuildId = guild_id;
		}
		else
		{
			std::string guild_id_str;
			if (utils::TryGetJsonValue(data, guild_id_str, "guild_id"))
			{
				Guild_t const &guild = GuildManager::Get()->FindGuildById(guild_id_str);
				m_GuildId = guild->GetPawnId();
				guild->AddChannel(pawn_id);
			}
			else
			{
				Logger::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"guild_id\" in \"{}\"", data.dump());
			}
		}
		Update(data);
	}
}

void Channel::Update(json const &data)
{
	utils::TryGetJsonValue(data, m_Name, "name"),
	utils::TryGetJsonValue(data, m_Topic, "topic"),
	utils::TryGetJsonValue(data, m_Position, "position"),
	utils::TryGetJsonValue(data, m_IsNsfw, "nsfw");
}

void Channel::UpdateParentChannel(Snowflake_t const &parent_id)
{
	Channel_t const &channel = ChannelManager::Get()->FindChannelById(parent_id);
	if (!channel)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"can't update parent channel \"parent_id\" not cached \"{}\"", parent_id);
		return;
	}
	m_ParentId = channel->GetPawnId();
}

void Channel::SendMessage(std::string &&msg, pawn_cb::Callback_t &&cb)
{
	json data = {
		{ "content", std::move(msg) }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Http::ResponseCb_t response_cb;
	if (cb)
	{
		response_cb = [cb](Http::Response response)
		{
			Logger::Get()->Log(LogLevel::DEBUG,
				"channel message create response: status {}; body: {}; add: {}",
				response.status, response.body, response.additional_data);
			if (response.status / 100 == 2) // success
			{
				auto msg_json = json::parse(response.body);
				PawnDispatcher::Get()->Dispatch([cb, msg_json]() mutable
				{
					auto msg = MessageManager::Get()->Create(msg_json);
					if (msg != INVALID_MESSAGE_ID)
					{
						MessageManager::Get()->SetCreatedMessageId(msg);
						cb->Execute();
						MessageManager::Get()->SetCreatedMessageId(INVALID_MESSAGE_ID);

						MessageManager::Get()->Delete(msg);
					}
				});
			}
		};
	}

	Network::Get()->Http().Post(fmt::format("/channels/{:s}/messages", GetId()), json_str,
		std::move(response_cb));
}

void Channel::SetChannelName(std::string const &name)
{
	json data = {
		{ "name", name }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelTopic(std::string const &topic)
{
	json data = {
		{ "topic", topic }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelPosition(int const position)
{
	json data = {
		{ "position", position }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelNsfw(bool const is_nsfw)
{
	json data = {
		{ "nsfw", is_nsfw }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::SetChannelParentCategory(Channel_t const &parent)
{
	if (parent->GetType() != Type::GUILD_CATEGORY)
		return;

	json data = {
		{ "parent_id", parent->GetId() }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Network::Get()->Http().Patch(fmt::format("/channels/{:s}", GetId()), json_str);
}

void Channel::DeleteChannel()
{
	Network::Get()->Http().Delete(fmt::format("/channels/{:s}", GetId()));
}

void Channel::SendEmbeddedMessage(const Embed_t & embed, std::string&& msg, pawn_cb::Callback_t&& cb)
{
	json data = {
		{ "content", std::move(msg) },
		{ "embed", {
			{ "title", embed->GetTitle() },
			{ "description", embed->GetDescription() },
			{ "url", embed->GetUrl() },
			{ "timestamp", embed->GetTimestamp() },
			{ "color", embed->GetColor() },
			{ "footer", {
				{"text", embed->GetFooterText()},
				{"icon_url", embed->GetFooterIconUrl()},
				{"proxy_icon_url", embed->GetFooterIconProxyUrl()}
			}},
			{"thumbnail", {
				{"url", embed->GetThumbnailUrl()},
				{"proxy_url", embed->GetThumbnailProxyUrl()},
				{"height", embed->GetThumbnailHeight()},
				{"width", embed->GetThumbnailWidth()}
			}}
		}}
	};

	// Add fields (if any).
	if (embed->GetFields().size())
	{
		json field_array = json::array();
		for (const auto& i : embed->GetFields())
		{
			field_array.push_back({ { "name", i._name }, {"value", i._value}, {"inline", i._inline_} });
		}
		json field_data = {
			{"fields", field_array}
		};
		data["embed"].insert(field_data.begin(), field_data.end());
	}

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);

	Http::ResponseCb_t response_cb;
	if (cb)
	{
		response_cb = [cb](Http::Response response)
		{
			Logger::Get()->Log(LogLevel::DEBUG,
				"channel message create response: status {}; body: {}; add: {}",
				response.status, response.body, response.additional_data);
			if (response.status / 100 == 2) // success
			{
				auto msg_json = json::parse(response.body);
				PawnDispatcher::Get()->Dispatch([cb, msg_json]() mutable
					{
						auto msg = MessageManager::Get()->Create(msg_json);
						if (msg != INVALID_MESSAGE_ID)
						{
							MessageManager::Get()->SetCreatedMessageId(msg);
							cb->Execute();
							MessageManager::Get()->SetCreatedMessageId(INVALID_MESSAGE_ID);

							MessageManager::Get()->Delete(msg);
						}
					});
			}
		};
	}

	Network::Get()->Http().Post(fmt::format("/channels/{:s}/messages", GetId()), json_str,
		std::move(response_cb));
}

void ChannelManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_CREATE, [](json const &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(data);
			if (channel_id == INVALID_CHANNEL_ID)
				return;

			// forward DCC_OnChannelCreate(DCC_Channel:channel);
			pawn_cb::Error error;
			pawn_cb::Callback::CallFirst(error, "DCC_OnChannelCreate", channel_id);
		});
	});
	
	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_UPDATE, [](json const &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			Snowflake_t sfid;
			if (!utils::TryGetJsonValue(data, sfid, "id"))
			{
				Logger::Get()->Log(LogLevel::ERROR,
					"invalid JSON: expected \"id\" in \"{}\"", data.dump());
				return;
			}

			Channel_t const &channel = ChannelManager::Get()->FindChannelById(sfid);
			if (!channel)
			{
				Logger::Get()->Log(LogLevel::ERROR,
					"can't update channel: channel id \"{}\" not cached", sfid);
				return;
			}

			channel->Update(data);
			if (channel->GetType() != Channel::Type::GUILD_CATEGORY)
			{
				Snowflake_t parent_id;
				if (!utils::TryGetJsonValue(data, parent_id, "parent_id"))
				{
					Logger::Get()->Log(LogLevel::ERROR, "invalid JSON: expected \"parent_id\" in \"{}\"", data.dump());
					return;
				}
				channel->UpdateParentChannel(parent_id);
			}

			// forward DCC_OnChannelUpdate(DCC_Channel:channel);
			pawn_cb::Error error;
			pawn_cb::Callback::CallFirst(error, "DCC_OnChannelUpdate", channel->GetPawnId());
		});
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::CHANNEL_DELETE, [](json const &data)
	{
		ChannelManager::Get()->DeleteChannel(data);
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json const &data)
	{
		static const char *PRIVATE_CHANNEL_KEY = "private_channels";
		if (utils::IsValidJson(data, PRIVATE_CHANNEL_KEY, json::value_t::array))
		{
			for (auto const &c : data.at(PRIVATE_CHANNEL_KEY))
				AddChannel(c);
		}
		else
		{
			Logger::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"{}\" in \"{}\"", PRIVATE_CHANNEL_KEY, data.dump());
		}
		m_Initialized++;
	});
}

bool ChannelManager::IsInitialized()
{
	return m_Initialized == m_InitValue;
}

bool ChannelManager::CreateGuildChannel(Guild_t const &guild,
	std::string const &name, Channel::Type type, pawn_cb::Callback_t &&cb)
{
	json data = {
		{ "name", name },
		{ "type", static_cast<int>(type) }
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return false;
	}

	Network::Get()->Http().Post(fmt::format("/guilds/{:s}/channels", guild->GetId()), json_str,
		[this, cb](Http::Response r)
	{
		Logger::Get()->Log(LogLevel::DEBUG,
			"channel create response: status {}; body: {}; add: {}",
			r.status, r.body, r.additional_data);
		if (r.status / 100 == 2) // success
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(json::parse(r.body));
			if (channel_id == INVALID_CHANNEL_ID)
				return;

			if (cb)
			{
				PawnDispatcher::Get()->Dispatch([=]()
				{
					m_CreatedChannelId = channel_id;
					cb->Execute();
					m_CreatedChannelId = INVALID_CHANNEL_ID;
				});
			}
		}
	});

	return true;
}

ChannelId_t ChannelManager::AddChannel(json const &data, GuildId_t guild_id/* = 0*/)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_CHANNEL_ID;
	}

	Channel_t const &channel = FindChannelById(sfid);
	if (channel)
		return channel->GetPawnId(); // channel already exists

	ChannelId_t id = 1;
	while (m_Channels.find(id) != m_Channels.end())
		++id;

	if (!m_Channels.emplace(id, Channel_t(new Channel(id, data, guild_id))).second)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"can't create channel: duplicate key '{}'", id);
		return INVALID_CHANNEL_ID;
	}

	Logger::Get()->Log(LogLevel::INFO, "successfully added channel with id '{}'", id);
	return id;
}

void ChannelManager::DeleteChannel(json const &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	PawnDispatcher::Get()->Dispatch([this, sfid]()
	{
		Channel_t const &channel = FindChannelById(sfid);
		if (!channel)
		{
			Logger::Get()->Log(LogLevel::ERROR,
				"can't delete channel: channel id \"{}\" not cached", sfid);
			return;
		}

		// forward DCC_OnChannelDelete(DCC_Channel:channel);
		pawn_cb::Error error;
		pawn_cb::Callback::CallFirst(error, "DCC_OnChannelDelete", channel->GetPawnId());

		Guild_t const &guild = GuildManager::Get()->FindGuild(channel->GetGuildId());
		if (guild)
			guild->RemoveChannel(channel->GetPawnId());

		m_Channels.erase(channel->GetPawnId());
	});

}

Channel_t const &ChannelManager::FindChannel(ChannelId_t id)
{
	static Channel_t invalid_channel;
	auto it = m_Channels.find(id);
	if (it == m_Channels.end())
		return invalid_channel;
	return it->second;
}

Channel_t const &ChannelManager::FindChannelByName(std::string const &name)
{
	static Channel_t invalid_channel;
	for (auto const &c : m_Channels)
	{
		Channel_t const &channel = c.second;
		if (channel->GetName().compare(name) == 0)
			return channel;
	}
	return invalid_channel;
}

Channel_t const &ChannelManager::FindChannelById(Snowflake_t const &sfid)
{
	static Channel_t invalid_channel;
	for (auto const &c : m_Channels)
	{
		Channel_t const &channel = c.second;
		if (channel->GetId().compare(sfid) == 0)
			return channel;
	}
	return invalid_channel;
}
