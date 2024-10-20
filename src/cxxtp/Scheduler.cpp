#include "cxxtp/Scheduler.hpp"
#include <mutex>
namespace cxxtp {

Scheduler::Scheduler(unsigned numThreads)
    : _workers(), _mux(), _buffer(), _selfTasks(),
      _mainId(std::this_thread::get_id()), _nextAllocWorker() {
  for (unsigned i = 0; i < numThreads - 1; ++i) {
    Worker *worker = new Worker(*this);
    _workers[worker->getThreadId()] = worker;
  }
  _nextAllocWorker = _workers.begin();
}

Worker *Scheduler::_getNextWorker() {
  assert(_mainId == std::this_thread::get_id());
  if (_nextAllocWorker == _workers.end())
    _nextAllocWorker = _workers.begin();
  auto res = _nextAllocWorker->second;
  _nextAllocWorker++;
  return res;
}

bool Scheduler::_tryAllocTasksToWorkers() {
  assert(_mainId == std::this_thread::get_id());
  while (!_buffer.empty()) {
    auto &task = _buffer.front();
    Worker *selected = _getNextWorker();
    while (!selected->trySubmit(task)) {
      selected = _getNextWorker();
    }
    _buffer.pop();
  }
  return true;
}

void Scheduler::_schedulerOnce() {
  assert(_mainId == std::this_thread::get_id());
  _tryAllocTasksToWorkers();
  if (auto task = _selfTasks.tryPop(); task.has_value()) {
    task.value()();
  }
}

Scheduler::~Scheduler() {
  for (auto &[id, worker] : _workers) {
    worker->detach();
    delete worker;
  }
}
} // namespace cxxtp
