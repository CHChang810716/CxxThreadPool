#include "cxxtp/Scheduler.hpp"

#include <mutex>
namespace cxxtp {

Scheduler::Scheduler(unsigned numThreads)
    : _workers(),
      _mux(),
      _buffer(),
      _selfTasks(),
      _mainId(std::this_thread::get_id()),
      _nextAllocWorker() {
  for (unsigned i = 0; i < numThreads - 1; ++i) {
    Worker *worker = new Worker(*this);
    _workers[worker->getThreadId()] = worker;
  }
  _nextAllocWorker = _workers.begin();
}

Worker *Scheduler::_getNextWorker() {
  assert(_mainId == std::this_thread::get_id());
  if (_nextAllocWorker == _workers.end()) {
    _nextAllocWorker = _workers.begin();
    return nullptr;
  }
  auto res = _nextAllocWorker->second;
  _nextAllocWorker++;
  return res;
}

bool Scheduler::_tryAllocBufTaskToNextWorker(Task &task) {
  Worker *minTasksWorker = nullptr;
  // Try assign task to the most leisurely worker.
  for (auto &[tid, worker] : _workers) {
    if (!minTasksWorker) {
      minTasksWorker = worker;
      continue;
    }
    minTasksWorker =
        worker->getNumPendingTasks() < minTasksWorker->getNumPendingTasks()
            ? worker
            : minTasksWorker;
  }
  // TODO: move task
  if (minTasksWorker &&
      minTasksWorker->getNumPendingTasks() < getNumPendingTasks()) {
    if (minTasksWorker->trySubmit(task)) return true;
  } else {
    if (_selfTasks.tryPush(task)) return true;
  }
  // Failed, try to find any workable worker.
  for (unsigned i = 0; i < _workers.size() + 1; ++i) {
    Worker *selected = _getNextWorker();
    if (selected) {
      if (selected->trySubmit(task))
        return true;
      else
        continue;
    } else {
      if (_selfTasks.tryPush(task))
        return true;
      else
        continue;
    }
  }
  return false;
}

bool Scheduler::_tryAllocTasksToWorkers(std::mutex& mux, std::queue<Task>& q) {
  assert(_mainId == std::this_thread::get_id());
  std::unique_lock<std::mutex> lock(mux, std::try_to_lock);
  if (!lock.owns_lock()) return false;
  while (!q.empty()) {
    auto &task = q.front();
    if (_tryAllocBufTaskToNextWorker(task)) {
      q.pop();
      continue;
    }
    return false;
  }
  return true;
}

void Scheduler::_schedulerOnce() {
  assert(_mainId == std::this_thread::get_id());
  _tryAllocTasksToWorkers(_mux, _buffer);
  if (auto task = _selfTasks.tryPop(); task.has_value()) {
    task.value()();
  }
  // _tryAllocTasksToWorkers(_suspendMux, _suspended);
  if (!Worker::suspendQueue().empty()) {
    Worker::suspendQueue().front()();
    Worker::suspendQueue().pop();
  }
}

void Scheduler::_schedImpl(Task &&t) {
  assert(t);
  if (std::this_thread::get_id() == _mainId) {
    while (!_tryAllocBufTaskToNextWorker(t))
      ;
  } else {
    _buffer.emplace(std::move(t));
  }
}

void Scheduler::sched(Task &&t) {
  std::lock_guard<std::mutex> lock(_mux);
  _schedImpl(std::move(t));
}

void Scheduler::suspend(Task &&task) {
  std::lock_guard<std::mutex> lock(_suspendMux);
  _suspended.push(std::move(task));
}

Scheduler::~Scheduler() {
  for (auto &[id, worker] : _workers) {
    worker->detach();
    delete worker;
  }
}
}  // namespace cxxtp
