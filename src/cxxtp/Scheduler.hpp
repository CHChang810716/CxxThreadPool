#pragma once
#include <cassert>
#include <chrono>
#include <coroutine>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "cxxtp/ContextList.hpp"
#include "cxxtp/SchedCoroClient.hpp"
#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/Timer.hpp"
#include "cxxtp/Worker.hpp"
#include "cxxtp/ts_queue/LockQueue.hpp"
#include "cxxtp/ts_queue/Status.hpp"
#include "cxxtp/ts_queue/TryLockQueue.hpp"

namespace cxxtp {

class Scheduler : public Worker {

 public:
  Scheduler(unsigned numThreads);
  ~Scheduler();

  template <class FuncCtx, class... Args>
  auto async(FuncCtx&& fctx, Args&&... args) {
    // 0-0. the return type of f should be coroutine
    // 0-1. the promise of coroutine type should have constructor to
    // capture the Scheduler
    // 1. put the f to a list member of Scheduler
    // 2. call the f from the list, a coroutine object returned -> co
    // 3. put co to queue
    // 4. in worker, pop queue and call co.resume()
    auto fut = [&]() {
      if constexpr (std::is_function_v<
                        std::remove_cvref_t<FuncCtx>> ||
                    std::is_lvalue_reference_v<FuncCtx>) {
        SchedCoroClient agent(this);
        return fctx(agent, std::forward<Args>(args)...);
      } else {
        auto pfctx =
            _liveContexts.append(std::forward<FuncCtx>(fctx));
        SchedCoroClient agent(this, pfctx);
        return pfctx->operator()(
            agent, std::forward<Args>(args)...);  // *this -> agent
      }
    }();
    std::coroutine_handle<> coro = fut.getCoroutineHandle();
    submit(std::move(coro));
    return fut;
  }

  template <class Fut>
  decltype(auto) await(Fut& fut) {
    auto myid = std::this_thread::get_id();
    if (myid != this->getThreadId()) {
      throw std::runtime_error(
          "The scheduler await could only called from main thread "
          "and none coroutine context");
    }
    while (!fut.await_ready()) {
      _schedulerOnce();
    }
    if constexpr (!std::is_void_v<typename Fut::ValueType>) {
      return fut.await_resume();
    }
  }

  template <class... DuArgs>
  auto suspendFor(std::chrono::duration<DuArgs...> du) {
    using namespace std::chrono_literals;
    auto dutime = std::chrono::steady_clock::now() + du;
    return Timer(this, dutime);
  }

  void submit(Task&& t) {
    TaskTransRes tt;
    if (std::this_thread::get_id() == getThreadId()) {
      tt = _trySubmitToNextWorker(std::move(t), true);
      if (tt.has_value()) {
        auto tmp = _suspended.tryPush(std::move(tt.value()));
        assert(!tmp.has_value());
      }
    } else {
      auto& w = _workers[std::this_thread::get_id()];
      do {
        tt = w->trySubmit(std::move(t));
        assert(tt.status != ts_queue::TS_FULL);
        if (tt.status == ts_queue::TS_DONE)
          break;
        t = std::move(tt.value());
      } while (true);
    }
    // if (tt.has_value()) {
    //   _slowQ.push(std::move(tt.value()));
    // }
  }

  template <class FuncCtx>
  void deleteFuncCtx(FuncCtx* ptr) {
    _liveContexts.remove(ptr);
  }

 private:
  using WorkerMap = std::map<std::thread::id, Worker*>;
  using WorkerIter = WorkerMap::iterator;
  void _schedulerOnce();
  TaskTransRes _trySubmitToNextWorker(Task&& task,
                                      bool schedulerIsWorker);
  bool _allocSuspendedTasksToWorkers();
  Worker* _getNextWorker();
  WorkerMap _workers;
  std::vector<std::thread> _threads;
  ContextList _liveContexts;
  ts_queue::LockQueue<Task> _slowQ;
  WorkerIter _nextWorker;
};

using CoSchedApi = SchedCoroClient<Scheduler>;

}  // namespace cxxtp
