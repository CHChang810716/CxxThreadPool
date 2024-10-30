#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <optional>
#include <thread>

#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/ts_queue/CircularQueue.hpp"
#include "cxxtp/ts_queue/TryLockQueue.hpp"

namespace cxxtp {

constexpr unsigned MAX_WORKER_TASKS = 1024;

class Scheduler;

class Worker {
  friend Scheduler;

 public:
  explicit Worker();
  
  explicit Worker(std::thread& t);

  TaskTransRes trySubmit(Task&& t);

  std::thread::id getThreadId() const { return _tid; }

  void detach();

  size_t getNumPendingTasks() const {
    return _ready.size();
  }

 private:
  void _default_loop();
  std::thread::id _tid;
  std::atomic<bool> _enabled{false};
  ts_queue::CircularQueue<Task, MAX_WORKER_TASKS>
      _ready{};  // master thread in worker thread out
  ts_queue::TryLockQueue<Task, MAX_WORKER_TASKS>
      _suspended{};  // worker thread in master thread out
  std::thread* _thread;
};

}  // namespace cxxtp
