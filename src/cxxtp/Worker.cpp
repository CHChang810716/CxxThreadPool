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

std::optional<Task> Worker::trySubmit(Task&& t) {
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
