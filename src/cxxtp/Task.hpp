#pragma once
#include <functional>
#include <coroutine>
#include <future>

namespace cxxtp {

using Task = std::function<void(void)>;

} // namespace cxxtp
