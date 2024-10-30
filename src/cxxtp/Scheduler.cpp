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

TaskTransRes Scheduler::_trySubmitToNextWorker(
    Task&& task, bool schedulerIsWorker) {
  // if (_slowQ.size() > 0) {
  //   Worker* minTasksWorker = nullptr;
  //   // Try assign task to the most leisurely worker.
  //   for (auto& [tid, worker] : _workers) {
  //     if (tid == getThreadId() && !schedulerIsWorker)
  //       continue;
  //     if (!minTasksWorker) {
  //       minTasksWorker = worker;
  //       continue;
  //     }
  //     minTasksWorker = worker->getNumPendingTasks() <
  //                              minTasksWorker->getNumPendingTasks()
  //                          ? worker
  //                          : minTasksWorker;
  //   }
  //   // if the Scheduler self is chosen, the submit won't recursive,
  //   // because now we use the Worker pointer without virtual
  //   return minTasksWorker->trySubmit(std::move(task));
  // } else {
  //   return _getNextWorker()->trySubmit(std::move(task));
  // }
  for (unsigned i = 0; i < _workers.size(); ++i) {
    auto *worker = _getNextWorker();
    if (!schedulerIsWorker && worker == this)
      continue;
    auto tmp = worker->trySubmit(std::move(task));
    if (!tmp.has_value())
      return tmp;
    task = std::move(tmp.value());
  }
  return {std::move(task), ts_queue::TS_FULL};
}

bool Scheduler::_allocSuspendedTasksToWorkers() {
  assert(getThreadId() == std::this_thread::get_id());
  for (auto& [id, worker] : _workers) {
    auto num = worker->_suspended.size();
    for(unsigned i = 0; i < num; ++i) {
      auto task = worker->_suspended.tryPop();
      if (!task.has_value())
        break;
      auto tt = _trySubmitToNextWorker(std::move(task.value()), true);
      if (tt.has_value()) {
        _suspended.tryPush(std::move(tt.value()));
        return false;
      }
    }
  }
  
  // while (this->_suspended.size() > 0) {
  //   auto task = this->_suspended.tryPop();
  //   if (!task.has_value())
  //     return false;
  //   auto tt = _trySubmitToNextWorker(std::move(task.value()), false);
  //   if (!tt.has_value())
  //     continue;
  //   _slowQ.push(std::move(tt.value()));
  // }
  return true;
}

void Scheduler::_schedulerOnce() {
  assert(getThreadId() == std::this_thread::get_id());
  _allocSuspendedTasksToWorkers();
  if (auto task = _ready.tryPop(); task.has_value()) {
    task.value()();
  }
  // if ( _slowQ.size() > 0) {
  //   auto tmpSQ = _slowQ.takeAll();
  //   while (!tmpSQ.empty()) {
  //     auto task = std::move(tmpSQ.front());
  //     submit(std::move(task));
  //     tmpSQ.pop();
  //   }
  // }
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
