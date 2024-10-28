#include "cxxtp/Worker.hpp"
#include <cassert>
#include <thread>

namespace cxxtp {

Worker::Worker()
    : _tid(std::this_thread::get_id()),
      _enabled(false),
      _ready(),
      _suspended(),
      _thread(nullptr) {}

Worker::Worker(std::thread& t)
    : _tid(), _enabled(true), _ready(), _suspended(), _thread(nullptr) {
  t = std::thread([this]() {
    while (_enabled.load()) {
      if (auto task = _ready.tryPop(); task.has_value()) {
        task.value()();
      }
    }
  });
  _tid = t.get_id();
  _thread = &t;
}

void Worker::submit(Task&& t) {
  auto forcePush = [&](auto& q) {
    while (auto tmp = q.tryPush(std::move(t))) {
      t = std::move(tmp.value());
    }
  };
  if (std::this_thread::get_id() == getThreadId()) {
    forcePush(_suspended);
  } else {
    forcePush(_ready);
  }
}

void Worker::detach() {
  _enabled.store(false);
  if (_thread)
    _thread->detach();
}

}  // namespace cxxtp
