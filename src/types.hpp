#pragma once

#include <memory>
#include <functional>
#include <tuple>

using std::shared_ptr;
using std::unique_ptr;
using std::tuple;


class CCallback;


using Callback_t = shared_ptr<CCallback>;

using DispatchFunction_t = std::function < void() >;
