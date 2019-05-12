#include "Bot.hpp"

#include "Network.hpp"
#include "Http.hpp"
#include "WebSocket.hpp"
#include "PawnDispatcher.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "Guild.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <json.hpp>
#include <fmt/format.h>

#include <map>


void ThisBot::TriggerTypingIndicator(Channel_t const &channel)
{
	Network::Get()->Http().Post(fmt::format("/channels/{:s}/typing", channel->GetId()), "");
}

void ThisBot::SetNickname(Guild_t const &guild, std::string const &nickname)
{
	json data = {
		{ "nick", nickname },
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return;
	}

	Network::Get()->Http().Patch(
		fmt::format("/guilds/{:s}/members/@me/nick", guild->GetId()), json_str);
}

bool ThisBot::CreatePrivateChannel(User_t const &user, pawn_cb::Callback_t &&callback)
{
	json data = {
		{ "recipient_id", user->GetId() },
	};

	std::string json_str;
	if (!utils::TryDumpJson(data, json_str))
	{
		Logger::Get()->Log(LogLevel::ERROR, "can't serialize JSON: {}", json_str);
		return false;
	}

	Network::Get()->Http().Post("/users/@me/channels", json_str,
		[this, callback](Http::Response r)
	{
		Logger::Get()->Log(LogLevel::DEBUG,
			"DM channel create response: status {}; body: {}; add: {}",
			r.status, r.body, r.additional_data);
		if (r.status / 100 == 2) // success
		{
			auto const channel_id = ChannelManager::Get()->AddChannel(json::parse(r.body));
			if (channel_id == INVALID_CHANNEL_ID)
				return;

			if (callback)
			{
				PawnDispatcher::Get()->Dispatch([=]()
				{
					m_CreatedChannelId = channel_id;
					callback->Execute();
					m_CreatedChannelId = INVALID_CHANNEL_ID;
				});
			}
		}
	});

	return true;
}

std::string const &GetPresenceStatusString(ThisBot::PresenceStatus status)
{
	static const std::map<ThisBot::PresenceStatus, std::string> mapping{
		{ ThisBot::PresenceStatus::ONLINE, "online" },
		{ ThisBot::PresenceStatus::DO_NOT_DISTURB, "dnd" },
		{ ThisBot::PresenceStatus::IDLE, "idle" },
		{ ThisBot::PresenceStatus::INVISIBLE, "invisible" },
		{ ThisBot::PresenceStatus::OFFLINE, "offline" },
	};
	static std::string invalid;

	auto it = mapping.find(status);
	if (it == mapping.end())
		return invalid;

	return it->second;
}

bool ThisBot::SetPresenceStatus(PresenceStatus status)
{
	auto const &status_str = GetPresenceStatusString(status);
	if (status_str.empty())
		return false; // invalid status passed

	m_PresenceStatus = status;
	Network::Get()->WebSocket().UpdateStatus(status_str, m_ActivityName);
	return true;
}

void ThisBot::SetActivity(std::string const &name)
{
	m_ActivityName = name;

	Network::Get()->WebSocket().UpdateStatus(
		GetPresenceStatusString(m_PresenceStatus), m_ActivityName);
}
