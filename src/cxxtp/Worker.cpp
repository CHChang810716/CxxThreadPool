#include "cxxtp/Worker.hpp"
#include <atomic>
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
    _default_loop();
  });
  _tid = t.get_id();
  _thread = &t;
}

void Worker::_default_loop() {
  while (_enabled.load(std::memory_order_relaxed)) {
    if (auto task = _ready.tryPop(); task.has_value()) {
      task.value()();
    } else if (auto task = _suspended.tryPop(); task.has_value()) {
      task.value()();
    }
  }
}

TaskTransRes Worker::trySubmit(Task&& t) {
  if (std::this_thread::get_id() == getThreadId()) {
    return _suspended.tryPush(std::move(t));
  } else {
    return _ready.tryPush(std::move(t));
  }
}

void Worker::detach() {
  _enabled.store(false);
  if (_thread)
    _thread->detach();
}

}  // namespace cxxtp
