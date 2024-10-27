#pragma once
#include <cassert>
#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "cxxtp/ContextList.hpp"
#include "cxxtp/Future.hpp"
#include "cxxtp/SchedAgent.hpp"
#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/Timer.hpp"
#include "cxxtp/Worker.hpp"

namespace cxxtp {

class Scheduler {
  friend SchedAgent<Scheduler>;

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
    std::lock_guard<std::mutex> lock(_mux);
    auto *pfctx = _liveContexts.append(std::forward<FuncCtx>(fctx));
    SchedAgent agent(this, pfctx);
    auto fut = pfctx->operator()(agent);  // *this -> agent
    auto coro = fut.getCoroutineHandle();
    Task task = [coro]() { coro.resume(); };
    schedImpl(std::move(task));
    return fut;
  }

  void sched(Task &&task);

  template <class... DuArgs>
  auto sleep_for(std::chrono::duration<DuArgs...> du) {
    using namespace std::chrono_literals;
    auto dutime = std::chrono::steady_clock::now() + du;
    return Timer(this, dutime);
  }

  unsigned getNumPendingTasks() const { return _selfTasks.size(); }

  template <class T>
  T await(Future<T> &fut) {
    auto myid = std::this_thread::get_id();
    if (myid != _mainId) {
      throw std::runtime_error(
          "The scheduler await could only called from main thread");
    }
    while (!fut.await_ready()) {
      _schedulerOnce();
    }
    if constexpr (!std::is_void_v<T>) {
      return fut.await_resume();
    }
  }

 private:
  void schedImpl(Task &&task);

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
  ContextList _liveContexts;
};

}  // namespace cxxtp
