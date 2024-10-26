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
#include "cxxtp/ContextList.hpp"

namespace cxxtp {
class Scheduler {
 public:
  Scheduler(unsigned numThreads);
  ~Scheduler();

  template <class FuncCtx>
  auto async(FuncCtx &&fctx) {
    // 0-0. the return type of f should be coroutine
    // 0-1. the promise of coroutine type should have constructor to
    // capture the Scheduler
    // 1. put the f to a list member of Scheduler
    // 2. call the f from the list, a coroutine object returned -> co
    // 3. put co to queue
    // 4. in worker, pop queue and call co.resume()
    // 5. when resume finish, check co status
    //  if co is finished, call co.destroy() and destroy the related f in
    //  Scheduler list otherwise, reschedule the co:
    //  Scheduler._async_co(co)
    std::lock_guard<std::mutex> lock(_mux);
    auto *pfctx = _liveContexts.append(std::forward<FuncCtx>(fctx));
    auto coroTask = pfctx->operator()(*this);
    auto fut = coroTask.getFuture();
    if (std::this_thread::get_id() == _mainId) {
      while (!_tryAllocBufTaskToNextWorker(coroTask))
        ;
    } else {
      _buffer.emplace(std::move(coroTask));
    }
    return fut;
  }

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
  ContextList _liveContexts;
};

}  // namespace cxxtp
