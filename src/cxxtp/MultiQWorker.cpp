#include "cxxtp/MultiQWorker.hpp"
#include <atomic>
#include <cassert>
#include <thread>
#include "cxxtp/Task.hpp"
#include "cxxtp/ts_queue/Status.hpp"

namespace cxxtp {
MultiQWorker::MultiQWorker(unsigned qid,
                           std::vector<SuspendQueue>& qs)
    : _tid(std::this_thread::get_id()),
      _enabled(false),
      _ready(),
      _suspended(&qs[qid]),
      _thread(nullptr),
      _stealIt(qs, qid) {}

MultiQWorker::MultiQWorker(std::thread& t, unsigned qid,
                           std::vector<SuspendQueue>& qs)
    : _tid(),
      _enabled(true),
      _ready(),
      _suspended(&qs[qid]),
      _thread(nullptr),
      _stealIt(qs, qid) {
  t = std::thread([this]() { _defaultLoop(); });
  _tid = t.get_id();
  _thread = &t;
}

void MultiQWorker::_defaultLoop() {
  while (_enabled.load(std::memory_order_relaxed)) {
    _defaultWork(*this);
  }
}

void MultiQWorker::submit(Task&& t) {
  assert(t);
  TaskTransRes tmp = trySubmit(std::move(t));
  if (tmp.status != ts_queue::TS_DONE) {
    _pending.push(std::move(tmp.value()));
  }
}

TaskTransRes MultiQWorker::trySubmit(Task&& t) {
  assert(t);
  if (std::this_thread::get_id() == getThreadId()) {
    return _suspended->tryPush(std::move(t));
  } else {
    return _ready.tryPush(std::move(t));
  }
}

void MultiQWorker::detach() {
  _enabled.store(false);
  if (_thread)
    _thread->detach();
}

thread_local std::queue<Task> MultiQWorker::_pending;

}  // namespace cxxtp
