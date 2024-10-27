#pragma once
#include <cassert>
#include <concepts>
#include <coroutine>
#include <functional>
#include <future>
#include <stdexcept>
#include <type_traits>

#include "cxxtp/SchedAgent.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/Worker.hpp"

namespace cxxtp {

template <class T>
concept SchedulerProxy = std::copy_constructible<T> &&
    requires(T a, Task task) {
  {a.deleteFuncCtx()};
  {a.sched(std::move(task))};
};

template <class T>
struct PromiseBase {
  void return_value(T &&v) { value = std::forward<T>(v); }
  void yield_value(T &&v) { value = std::forward<T>(v); }
  T value;
};

template <>
struct PromiseBase<void> {
  void return_void() {}
  void yield_void() {}
};

template <class T, SchedulerProxy SchedulerAgent = SchedAgent<Scheduler>>
class Future {
 public:
  struct promise_type : public PromiseBase<T> {
    // TODO: operator new for memory efficiency
    // constructor forwarding https://godbolt.org/z/T59dcebPM
    template <class... Args>
    promise_type(SchedulerAgent sa, Args &&...args)
        : _schedulerAgent(sa) {}

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
    SchedulerAgent _schedulerAgent;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Future(const Handle handle, SchedulerAgent sa)
      : _internal(handle), _schedulerAgent(sa), _active(true) {}

  Future(const Future &) = delete;
  Future(Future &&other)
      : _internal(std::move(other._internal)),
        _schedulerAgent(std::move(other._schedulerAgent)),
        _active(std::move(other._active)) {
    other._active = false;
  }
  ~Future() {
    if (!_active) return;
    auto destruct = [coro = this->_internal, sa = this->_schedulerAgent](
                        auto self) mutable -> void {
      if (coro.done()) {
        coro.destroy();
        sa.deleteFuncCtx();
      } else {
        Worker::suspendQueue().push([self]() mutable { self(self); });
      }
    };
    destruct(destruct);
  }

  bool await_ready() { return _active && _internal.done(); }

  void await_suspend(std::coroutine_handle<> caller) {
    if (!_active) throw std::runtime_error("await on dead future");
    Worker::suspendQueue().push([caller, this]() {
      if (_internal.done())
        caller.resume();
      else
        await_suspend(caller);
    });
  }

  auto &&await_resume() {
    if (!_active) throw std::runtime_error("await on dead future");
    assert(_internal.done());
    if constexpr (!std::is_void_v<T>) {
      return _internal.promise().value;
    }
  }

  std::coroutine_handle<> getCoroutineHandle() const { return _internal; }

 private:
  Handle _internal;
  SchedulerAgent _schedulerAgent;
  bool _active;
};

}  // namespace cxxtp
