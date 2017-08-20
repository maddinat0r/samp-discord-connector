#pragma once

#include <string>
#include <memory>
#include "sdk.hpp"


using Snowflake_t = std::string;

using Guild_t = std::unique_ptr<class Guild>;
using GuildId_t = cell;

using User_t = std::unique_ptr<class User>;
using UserId_t = cell;

using Channel_t = std::unique_ptr<class Channel>;
using ChannelId_t = cell;

using Role_t = std::unique_ptr<class Role>;
using RoleId_t = cell;
