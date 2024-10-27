#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <thread>

#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

constexpr unsigned MAX_WORKER_TASKS = 512;
class Scheduler;

// task life cycle
// 1. pop from worker
// 2. run by worker
class Worker {
  friend Scheduler;

 public:
  Worker(Scheduler &sched);
  bool trySubmit(Task &t);
  std::thread::id getThreadId() const { return _t.get_id(); }
  void detach();
  size_t getNumPendingTasks() const { return _queue.size(); }
  static std::queue<Task> &suspendQueue();

 private:
  void _workerOnce();
  void _workerLoop();
  std::atomic<bool> _enabled;
  std::thread _t;
  TSQueue<Task, MAX_WORKER_TASKS> _queue;
  Scheduler *_scheduler;
};

}  // namespace cxxtp
