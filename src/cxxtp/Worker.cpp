#include "cxxtp/Worker.hpp"
#include <atomic>
#include <cassert>
#include <thread>
#include "cxxtp/Task.hpp"
#include "cxxtp/ts_queue/Status.hpp"

namespace cxxtp {

Worker::Worker()
    : _tid(std::this_thread::get_id()),
      _enabled(false),
      _ready(),
      _suspended(),
      _thread(nullptr) {}

Worker::Worker(std::thread& t)
    : _tid(),
      _enabled(true),
      _ready(),
      _suspended(),
      _thread(nullptr) {
  t = std::thread([this]() { _defaultLoop(); });
  _tid = t.get_id();
  _thread = &t;
}

void Worker::_defaultLoop() {
  while (_enabled.load(std::memory_order_relaxed)) {
    _defaultWork(*this);
  }
}

void Worker::submit(Task&& t) {
  assert(t);
  TaskTransRes tmp;
  if (std::this_thread::get_id() == getThreadId()) {
    tmp = _suspended.tryPush(std::move(t));
  } else {
    tmp = _ready.tryPush(std::move(t));
  }
  if (tmp.status != ts_queue::TS_DONE) {
    _pending.push(std::move(tmp.value()));
  }
}

void Worker::detach() {
  _enabled.store(false);
  if (_thread)
    _thread->detach();
}

thread_local std::queue<Task> Worker::_pending;

}  // namespace cxxtp
