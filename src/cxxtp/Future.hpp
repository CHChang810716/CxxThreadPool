#pragma once
#include <cassert>
#include <coroutine>
#include <functional>
#include <future>
#include <type_traits>

#include "cxxtp/Task.hpp"

namespace cxxtp {

template <class T>
concept SchedulerProxy = requires(T a, Task task) {
  {a.deleteFuncCtx()};
  {a.sched(task)};
};

template <class T>
struct PromiseBase {
  void return_value(T &&v) { value = std::forward<T>(v); }
  T value;
};

template <>
struct PromiseBase<void> {
  void return_void() {}
};

template <class T, SchedulerProxy SchedulerAgent>
class Future {
 public:
  struct promise_type : public PromiseBase<T> {
    // TODO: operator new for memory efficiency
    // constructor forwarding https://godbolt.org/z/T59dcebPM
    template<class... Args>
    promise_type(SchedulerAgent &sa, Args&&... args) : _schedulerAgent(&sa) {}

    Future<T, SchedulerAgent> get_return_object() {
      return Future<T, SchedulerAgent>{Handle::from_promise(*this),
                                       _schedulerAgent};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {
      // TODO: need handle this
    }

   private:
    SchedulerAgent *_schedulerAgent;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Future(const Handle handle, SchedulerAgent *sa)
      : _internal(handle), _schedulerAgent(sa) {}
  Future() = delete;
  Future(const Future &) = delete;
  Future(Future &&) = default;
  ~Future() {
    if (_internal) _internal.destroy();
    if (_schedulerAgent) _schedulerAgent->deleteFuncCtx();
  }

  bool await_ready() { return _internal.done(); }

  void await_suspend(std::coroutine_handle<> caller) {
    _schedulerAgent->sched([caller, this]() {
      if (_internal.done())
        caller.resume();
      else
        await_suspend(caller);
    });
  }

  auto &&await_resume() {
    assert(_internal.done());
    if constexpr (!std::is_void_v<T>) {
      return _internal.promise().value;
    }
  }

  std::coroutine_handle<> getCoroutineHandle() const { return _internal; }

 private:
  Handle _internal;
  SchedulerAgent *_schedulerAgent;
};

}  // namespace cxxtp
