#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <optional>
#include <thread>

#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/ts_queue/CircularQueue.hpp"
#include "cxxtp/ts_queue/STQueue.hpp"
#include "cxxtp/ts_queue/Status.hpp"
#include "cxxtp/ts_queue/TryLockQueue.hpp"

namespace cxxtp {

class Scheduler;

constexpr unsigned MAX_WORKER_TASKS = 1024;

class Worker {

  friend Scheduler;

 public:
  explicit Worker();

  explicit Worker(std::thread& t);

  void submit(Task&& t);

  TaskTransRes trySubmit(Task&& t);

  std::thread::id getThreadId() const { return _tid; }

  void detach();

  size_t getNumPendingTasks() const { return _ready.size(); }

 protected:
  void _defaultLoop();

  template <class Submitter>
  void _defaultWork(Submitter& s) {
    if (auto task = _ready.tryPop(); task.has_value()) {
      if (!_pending.empty()) {
        s.submit(std::move(_pending.front()));
        _pending.pop();
      }
      task.value()();
    } else {
      if (!_pending.empty()) {
        auto& t = _pending.front();
        t();
        _pending.pop();
      }
    }
  }

  std::thread::id _tid;

  std::atomic<bool> _enabled{false};

  /// master thread in worker thread out
  ts_queue::CircularQueue<Task, MAX_WORKER_TASKS> _ready{};

  /// worker thread in master thread out
  ts_queue::CircularQueue<Task, MAX_WORKER_TASKS> _suspended{};

  /// need reschedule tasks
  static thread_local std::queue<Task> _pending;

  std::thread* _thread;
};

}  // namespace cxxtp
