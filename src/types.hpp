#pragma once

#include <string>
#include <memory>
#include "sdk.hpp"


using Snowflake_t = std::string;

using Guild_t = std::unique_ptr<class Guild>;
using GuildId_t = cell;
const GuildId_t INVALID_GUILD_ID = 0;

using User_t = std::unique_ptr<class User>;
using UserId_t = cell;
const UserId_t INVALID_USER_ID = 0;

using Channel_t = std::unique_ptr<class Channel>;
using ChannelId_t = cell;
const ChannelId_t INVALID_CHANNEL_ID = 0;

using Message_t = std::unique_ptr<class Message>;
using MessageId_t = cell;
const MessageId_t INVALID_MESSAGE_ID = 0;

using Role_t = std::unique_ptr<class Role>;
using RoleId_t = cell;
const RoleId_t INVALID_ROLE_ID = 0;
