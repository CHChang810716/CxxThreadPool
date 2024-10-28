#include "cxxtp/Scheduler.hpp"

#include <mutex>
namespace cxxtp {

Scheduler::Scheduler(unsigned numThreads)
    : _workers(), _threads(), _nextAllocWorker(), _liveContexts() {
  _threads.resize(numThreads - 1);
  for (auto& t : _threads) {
    Worker* worker = new Worker(t);
    _workers[worker->getThreadId()] = worker;
  }
  _workers[getThreadId()] = this;
  _nextAllocWorker = _workers.begin();
}

Scheduler::~Scheduler() {
  for (auto& [id, worker] : _workers) {
    worker->detach();
    if (worker != this)
      delete worker;
  }
}

Worker* Scheduler::_getNextWorker() {
  assert(getThreadId() == std::this_thread::get_id());
  if (_nextAllocWorker == _workers.end()) {
    _nextAllocWorker = _workers.begin();
  }
  auto res = _nextAllocWorker->second;
  _nextAllocWorker++;
  return res;
}

void Scheduler::_submitToNextWorker(Task&& task, bool schedulerIsWorker) {
  Worker* minTasksWorker = nullptr;
  // Try assign task to the most leisurely worker.
  for (auto& [tid, worker] : _workers) {
    if (tid == getThreadId() && !schedulerIsWorker) continue;
    if (!minTasksWorker) {
      minTasksWorker = worker;
      continue;
    }
    minTasksWorker = worker->getNumPendingTasks() <
                             minTasksWorker->getNumPendingTasks()
                         ? worker
                         : minTasksWorker;
  }
  // if the Scheduler self is chosen, the submit won't recursive, 
  // because now we use the Worker pointer without virtual
  minTasksWorker->submit(std::move(task));
}

void Scheduler::_allocSuspendedTasksToWorkers() {
  assert(getThreadId() == std::this_thread::get_id());
  for (auto &[id, worker] : _workers) {
    while (auto task = worker->_suspended.tryPop()) {
      _submitToNextWorker(std::move(task.value()), false);
    }
  }
}

void Scheduler::_schedulerOnce() {
  assert(getThreadId() == std::this_thread::get_id());
  _allocSuspendedTasksToWorkers();
  if (auto task = _ready.tryPop(); task.has_value()) {
    task.value()();
  }
}

}  // namespace cxxtp
