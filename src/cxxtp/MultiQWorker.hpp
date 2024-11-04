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
#include "cxxtp/ts_queue/VersionQueue.hpp"

namespace cxxtp {

class Scheduler;

constexpr unsigned MAX_WORKER_TASKS = 1024;

class MultiQWorker {
  friend Scheduler;

 public:
  using ReadyQueue = ts_queue::CircularQueue<Task, MAX_WORKER_TASKS>;
  using SuspendQueue = ts_queue::VersionQueue<Task, MAX_WORKER_TASKS>;
  using SuspendQueues = std::vector<SuspendQueue>;
  using StealQueues = SuspendQueues;

 private:
  struct StealIterator {
    StealIterator() = default;
    StealIterator(StealQueues& qs, unsigned skipId)
        : _qs(nullptr), _it(), _skipIt() {
      reset(qs, skipId);
    }
    void reset(StealQueues& qs, unsigned skipId) {
      assert(skipId < qs.size());
      _qs = &qs;
      _it = _qs->begin() + skipId + 1;
      if (_it == _qs->end())
        _it = _qs->begin();
      _skipIt = _qs->begin() + skipId;
    }
    StealIterator& operator++() {
      ++_it;
      if (_it == _skipIt)
        ++_it;
      if (_it == _qs->end())
        _it = _qs->begin();
      return *this;
    }
    StealIterator operator++(int) {
      StealIterator tmp = *this;
      ++(*this);
      return tmp;
    }
    StealQueues::value_type& operator*() { return *_it; }
    StealQueues::value_type* operator->() { return &*_it; }
    operator bool() const {
      return _it != _qs->end() && _it != _skipIt;
    }

   private:
    StealQueues::iterator _it{};
    StealQueues::iterator _skipIt{};
    StealQueues* _qs{nullptr};
  };

 public:
  MultiQWorker() = default;

  explicit MultiQWorker(unsigned qid, SuspendQueues& qs);

  explicit MultiQWorker(std::thread& t, unsigned qid,
                        std::vector<SuspendQueue>& qs);

  void submit(Task&& t);

  TaskTransRes trySubmit(Task&& t);

  std::thread::id getThreadId() const { return _tid; }

  void detach();

  size_t getNumPendingTasks() const { return _ready.size(); }

 protected:
  void _defaultLoop();

  template <class Submitter>
  void _defaultWork(Submitter& s) {
    // If ready queue has task, run it and reschdule a pending queue task.
    // Otherwise, if pending queue has task, run it.
    // Otherwise, steal task from queue list.
    auto reschduleOnePending = [&]() {
      if (!_pending.empty()) {
        s.submit(std::move(_pending.front()));
        _pending.pop();
      }
    };
    if (auto task = _ready.tryPop(); task.has_value()) {
      reschduleOnePending();
      task.value()();
    } else if (!_pending.empty()) {
      auto& t = _pending.front();
      t();
      _pending.pop();
    } else {
      auto curStealIt = _stealIt;
      while (_stealIt) {
        if (auto task = _stealIt->tryPop(); task.has_value()) {
          reschduleOnePending();
          task.value()();
          break;
        }
        ++_stealIt;
        if (_stealIt == curStealIt) {
          break;
        }
      }
    }
  }

  std::thread::id _tid;

  std::atomic<bool> _enabled{false};

  /// master thread in worker thread out
  ReadyQueue _ready{};

  /// worker thread in master thread out
  SuspendQueue* _suspended;

  std::thread* _thread;

  StealIterator _stealIt;

  static thread_local std::queue<Task> _pending;
};

}  // namespace cxxtp
