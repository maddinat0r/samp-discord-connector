#pragma once

#include <memory>
#include <string>


class CCallback;
class CChannel;


using Callback_t = std::shared_ptr<CCallback>;
using Channel_t = std::unique_ptr<CChannel>;
using ChannelId_t = unsigned int;
using Snowflake_t = std::string;
