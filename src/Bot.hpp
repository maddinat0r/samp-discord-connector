#pragma once

#include "types.hpp"
#include "Singleton.hpp"
#include "Callback.hpp"


class ThisBot : public Singleton<ThisBot>
{
	friend class Singleton<ThisBot>;
public:
	enum class PresenceStatus : int
	{
		INVALID = 0,
		ONLINE,
		IDLE,
		DO_NOT_DISTURB,
		INVISIBLE,
		OFFLINE
	};

private:
	ThisBot() = default;
	~ThisBot() = default;

	ThisBot(ThisBot const &) = delete;
	ThisBot &operator=(ThisBot const &) = delete;

private:
	PresenceStatus m_PresenceStatus = PresenceStatus::ONLINE;
	std::string m_ActivityName;
	ChannelId_t m_CreatedChannelId = INVALID_CHANNEL_ID;

public:
	void TriggerTypingIndicator(Channel_t const &channel);
	void SetNickname(Guild_t const &guild, std::string const &nickname);

	bool CreatePrivateChannel(User_t const &user, pawn_cb::Callback_t &&callback);
	ChannelId_t GetCreatedPrivateChannelId() const
	{
		return m_CreatedChannelId;
	}

	bool SetPresenceStatus(PresenceStatus status);
	PresenceStatus GetPresenceStatus() const
	{
		return m_PresenceStatus;
	}

	void SetActivity(std::string const &name);
};
