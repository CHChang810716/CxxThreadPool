#pragma once
#include <cassert>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

// #include "cxxtp/FutureSet.hpp"
#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/Worker.hpp"

namespace cxxtp {
class Scheduler {
 public:
  Scheduler(unsigned numThreads);
  ~Scheduler();
  template <class Func>
  auto async(Func &&f) {
    std::lock_guard<std::mutex> lock(_mux);
    using Res = decltype(f(*this));
    using Promise = std::promise<Res>;
    using PromisePtr = std::shared_ptr<Promise>;
    PromisePtr p(new Promise());
    auto fut = p->get_future();
    Task task = [this, p, ff{std::move(f)}]() {
      if constexpr (std::is_void_v<Res>) {
        ff(*this);
        p->set_value();
      } else {
        p->set_value(ff(*this));
      }
    };
    if (std::this_thread::get_id() == _mainId) {
      while (!_tryAllocBufTaskToNextWorker(task))
        ;
    } else {
      _buffer.emplace(std::move(task));
    }
    return fut;
  }

  template <class T>
  auto await(std::future<T> &fut) {
    _only_await(fut);
    return fut.get();
  }

  void yield() {
    auto myid = std::this_thread::get_id();
    auto iter = _workers.find(myid);
    if (iter == _workers.end()) {
      if (_mainId == myid) {
        _schedulerOnce();
      }
    } else {
      iter->second->_workerOnce();
    }
  }

  template <class... DuArgs>
  void yield_for(std::chrono::duration<DuArgs...> du) {
    auto start = std::chrono::steady_clock::now();
    while ((start + du) > std::chrono::steady_clock::now()) {
      yield();
    }
  }

  // template <class T>
  // std::list<T> await(FutureSet<T> &futSet) {
  //   std::list<T> res;
  //   auto &futs = futSet._data;
  //   for (; !futs.empty(); futs.pop_front()) {
  //     auto &fut = futs.front();
  //     _only_await(fut);
  //     res.push_back(fut.get());
  //   }
  //   return res;
  // }

  // void await(FutureSet<void> &futSet);

  // template <class... T>
  // std::tuple<T...> await(FutureTuple<T...> &futSet) {
  //   return std::apply(
  //       [&](auto &... fut) { return std::make_tuple(await(fut)...); },
  //       futSet);
  // }

  unsigned getNumPendingTasks() const {
    return _selfTasks.size() + _numStackLevel;
  }

 private:
  template <class T>
  void _only_await(std::future<T> &fut) {
    auto myid = std::this_thread::get_id();
    auto iter = _workers.find(myid);
    auto isFutReady = [&]() {
      return fut.wait_for(std::chrono::seconds(0)) ==
                 std::future_status::ready ||
             !fut.valid();
    };
    if (iter == _workers.end()) {
      if (_mainId == myid) {
        while (!isFutReady()) {
          _schedulerOnce();
        }
        assert(fut.valid());
      } else {
        fut.wait();
      }
    } else {
      while (!isFutReady()) {
        iter->second->_workerOnce();
      }
      assert(fut.valid());
    }
  }
  using WorkerMap = std::map<std::thread::id, Worker *>;
  bool _tryAllocTasksToWorkers();
  void _schedulerOnce();
  Worker *_getNextWorker();
  bool _tryAllocBufTaskToNextWorker(Task &task);
  WorkerMap _workers;
  std::mutex _mux;
  std::queue<Task> _buffer;
  TSQueue<Task, MAX_WORKER_TASKS> _selfTasks;
  std::thread::id _mainId;
  WorkerMap::iterator _nextAllocWorker;
  unsigned _numStackLevel;
};

}  // namespace cxxtp
