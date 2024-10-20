#include "cxxtp/Worker.hpp"
#include "cxxtp/Scheduler.hpp"

namespace cxxtp {

Worker::Worker(Scheduler &sched)
    : _enabled(false), _queue(), _scheduler(&sched), _t() {
  _enabled.store(true);
  _t = std::thread([this]() { _workerLoop(); });
}

bool Worker::trySubmit(Task &t) { return _queue.tryPush(t); }
void Worker::_workerOnce() {
  if (auto task = _queue.tryPop(); task.has_value()) {
    task.value()();
  }
}
void Worker::_workerLoop() {
  while (_enabled.load()) {
    _workerOnce();
  }
}
void Worker::detach() {
  _enabled.store(false);
  _t.detach();
}
} // namespace cxxtp
