#pragma once
#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <coroutine>

#include "cxxtp/Future.hpp"
#include "cxxtp/Scheduler.hpp"

namespace cxxtp {

class Profiler {

 public:
  Profiler() = default;
  explicit Profiler(unsigned msInterval)
      : _schedCnts(),
        _interval(std::chrono::milliseconds(msInterval)) {}

  ~Profiler() = default;
  auto make() {
    return [&](auto sched) -> cxxtp::Future<void> {
      while (true) {
        auto id = std::this_thread::get_id();
        _schedCnts[id]++;
        co_await sched.suspendFor(_interval);
      }
    };
  }

  void print(std::ostream& out) {
    for (auto& [k, v] : _schedCnts) {
      out << "thread: " << std::hex << k << std::dec
          << " times: " << v << std::endl;
    }
    _schedCnts.clear();
  }

 private:
  std::map<std::thread::id, unsigned> _schedCnts;
  std::chrono::milliseconds _interval;
};
}  // namespace cxxtp

template<class... Args>
struct std::coroutine_traits<cxxtp::Future<void>, cxxtp::Profiler&, Args...> {
  using promise_type = cxxtp::Future<void>::promise_type;
};