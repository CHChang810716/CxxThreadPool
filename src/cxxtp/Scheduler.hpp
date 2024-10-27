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
#include <utility>
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
  using Agent = SchedAgent<Scheduler>;
  Scheduler(unsigned numThreads);
  ~Scheduler();

  template <class FuncCtx, class... Args>
  auto async(FuncCtx &&fctx, Args &&...args) {
    // 0-0. the return type of f should be coroutine
    // 0-1. the promise of coroutine type should have constructor to
    // capture the Scheduler
    // 1. put the f to a list member of Scheduler
    // 2. call the f from the list, a coroutine object returned -> co
    // 3. put co to queue
    // 4. in worker, pop queue and call co.resume()

    std::lock_guard<std::mutex> lock(_mux);

    auto fut =
        _async_0(std::is_function<std::remove_cvref_t<FuncCtx>>(),
                 std::forward<FuncCtx>(fctx), std::forward<Args>(args)...);
    auto coro = fut.getCoroutineHandle();
    Task task = [coro]() { coro.resume(); };
    _schedImpl(std::move(task));
    return fut;
  }

  void sched(Task &&task);

  void suspend(Task &&task);

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
  template <class FuncCtx, class... Args>
  auto _async_0(std::true_type, FuncCtx &&fctx, Args &&...args) {
    SchedAgent agent(this);
    auto fut = fctx(agent, std::forward<Args>(args)...);
    return fut;
  }
  template <class FuncCtx, class... Args>
  auto _async_0(std::false_type, FuncCtx &&fctx, Args &&...args) {
    auto pfctx = _liveContexts.append(std::forward<FuncCtx>(fctx));
    SchedAgent agent(this, pfctx);
    auto fut = pfctx->operator()(
        agent, std::forward<Args>(args)...);  // *this -> agent
    return fut;
  }
  void _schedImpl(Task &&task);

  using WorkerMap = std::map<std::thread::id, Worker *>;
  bool _tryAllocTasksToWorkers(std::mutex& mux, std::queue<Task>& q);
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
  std::mutex _suspendMux;
  std::queue<Task> _suspended;
};

using SchedApi = Scheduler::Agent;

}  // namespace cxxtp
