#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <optional>
#include <thread>

#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

constexpr unsigned MAX_WORKER_TASKS = 512;

class Scheduler;

class Worker {
  friend Scheduler;

 public:
  explicit Worker();
  explicit Worker(std::thread& t);

  std::optional<Task> trySubmit(Task&& t);

  std::thread::id getThreadId() const { return _tid; }

  void detach();

  size_t getNumPendingTasks() const {
    return _ready.size();
  }

 private:
  std::thread::id _tid;
  std::atomic<bool> _enabled{false};
  TSQueue<Task, MAX_WORKER_TASKS>
      _ready{};  // master thread in worker thread out
  TSQueue<Task, MAX_WORKER_TASKS>
      _suspended{};  // worker thread in master thread out
  std::thread* _thread;
};

}  // namespace cxxtp
