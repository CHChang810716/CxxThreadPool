#pragma once
#include <coroutine>
#include <functional>
#include <future>
#include <optional>

#include "cxxtp/ts_queue/Status.hpp"
namespace cxxtp {

using Task = std::function<void(void)>;

using TaskTransRes = ts_queue::TransRes<Task>;

}  // namespace cxxtp
