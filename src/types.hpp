#pragma once

#include <memory>
#include <tuple>

using std::shared_ptr;
using std::unique_ptr;
using std::tuple;


class CCallback;


using Callback_t = shared_ptr<CCallback>;

