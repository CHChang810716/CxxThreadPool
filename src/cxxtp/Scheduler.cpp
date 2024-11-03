#include "cxxtp/Scheduler.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include "cxxtp/Task.hpp"
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

void Scheduler::_submitToSelf(Task&& task) {
  if (auto trans = _ready.tryPush(std::move(task));
      trans.has_value()) {
    _pending.push(std::move(trans.value()));
  }
}

TaskTransRes Scheduler::_trySubmitToSelf(Task&& task) {
  return _ready.tryPush(std::move(task));
}

void Scheduler::_submitToNextWorker(Task&& task,
                                    bool schedulerIsWorker) {
  auto* worker = _getNextWorker(!schedulerIsWorker);
  if (worker == this)
    _submitToSelf(std::move(task));
  else
    worker->submit(std::move(task));
}

TaskTransRes Scheduler::_trySubmitToNextWorker(
    Task&& task, bool schedulerIsWorker) {

  auto tryTimes = _workers.size() - !schedulerIsWorker;
  TaskTransRes tmp;
  for (unsigned i = 0; i < tryTimes; ++i) {
    auto* worker = _getNextWorker(!schedulerIsWorker);
    if (worker == this)
      tmp = _trySubmitToSelf(std::move(task));
    else
      tmp = worker->trySubmit(std::move(task));
    if (!tmp.has_value())
      return tmp;
    task = std::move(tmp.value());
  }
  tmp = {std::move(task), ts_queue::TS_RACE};
  return tmp;
}

void Scheduler::_allocSuspendedTasksToWorkers() {
  assert(getThreadId() == std::this_thread::get_id());
  for (auto& [id, worker] : _workers) {
    auto num = worker->_suspended.size();
    for (unsigned i = 0; i < num; ++i) {
      auto task = worker->_suspended.tryPop();
      if (!task.has_value())
        break;
      task = _trySubmitToNextWorker(std::move(task.value()), true);
      // printStatus();
      if (task.has_value()) {  // all worker full
        _pending.push(std::move(task.value()));
        return;
      }
    }
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

Worker* Scheduler::_getNextWorker(bool skipScheduler) {
  auto* worker = _getNextWorker();
  if (worker == this && skipScheduler) {
    worker = _getNextWorker();
  }
  return worker;
}

static std::mutex mux;
void Scheduler::printStatus() {
  std::lock_guard<std::mutex> loc(mux);
  std::cout << "timestamp: " << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
  for (auto& [id, pw] : _workers) {
    std::cout << "  [" << std::hex << id << std::dec << "]: " << std::endl;
    std::cout << "    ready:     " << pw->_ready.size() << std::endl;
    std::cout << "    suspended: " << pw->_suspended.size() << std::endl;
  }
}
}  // namespace cxxtp
