#include "cxxtp/Scheduler.hpp"

#include <mutex>
#include "cxxtp/ts_queue/Status.hpp"
namespace cxxtp {

Scheduler::Scheduler(unsigned numThreads)
    : _workers(), _threads(), _liveContexts(), _nextWorker() {
  _threads.resize(numThreads - 1);
  for (auto& t : _threads) {
    Worker* worker = new Worker(t);
    _workers[worker->getThreadId()] = worker;
  }
  _workers[getThreadId()] = this;
  _nextWorker = _workers.begin();
}

Scheduler::~Scheduler() {
  for (auto& [id, worker] : _workers) {
    worker->detach();
    if (worker != this)
      delete worker;
  }
}

void Scheduler::_submitToNextWorker(Task&& task,
                                    bool schedulerIsWorker) {
  auto* worker = _getNextWorker();
  if (!schedulerIsWorker && worker == this)
    worker = _getNextWorker();
  worker->submit(std::move(task));
}

void Scheduler::_allocSuspendedTasksToWorkers() {
  assert(getThreadId() == std::this_thread::get_id());
  for (auto& [id, worker] : _workers) {
    auto num = worker->_suspended.size();
    auto task = worker->_suspended.tryPop();
    if (!task.has_value())
      continue;
    _submitToNextWorker(std::move(task.value()), true);
  }
}

void Scheduler::_schedulerOnce() {
  assert(getThreadId() == std::this_thread::get_id());
  _allocSuspendedTasksToWorkers();
  _defaultWork(*this);
}

Worker* Scheduler::_getNextWorker() {
  auto res = _nextWorker;
  _nextWorker++;
  if (_nextWorker == _workers.end()) {
    _nextWorker = _workers.begin();
  }
  return res->second;
}

}  // namespace cxxtp
