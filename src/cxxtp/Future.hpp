#pragma once
#include <cassert>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <functional>
#include <future>
#include <stdexcept>
#include <type_traits>

#include "cxxtp/SchedCoroClient.hpp"
#include "cxxtp/Scheduler.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/Worker.hpp"

namespace cxxtp {

template <class T>
concept SchedulerProxy = std::copy_constructible<T> &&
    requires(T a, Task task) {
  {a.deleteFuncCtx()};
  {a.submit(std::move(task))};
};

template <class T>
struct PromiseBase {
  void return_value(T&& v) { value = std::forward<T>(v); }
  void yield_value(T&& v) { value = std::forward<T>(v); }
  T value;
};

template <>
struct PromiseBase<void> {
  void return_void() {}
  void yield_void() {}
};

template <class T, SchedulerProxy SP = CoSchedApi>
class Future {
 public:
  using ValueType = T;
  struct promise_type : public PromiseBase<T> {
    // TODO: operator new for memory efficiency
    // constructor forwarding https://godbolt.org/z/T59dcebPM
    template <class... Args>
    promise_type(SP sp, Args&&... args) : _sp(sp) {}

    Future<T, SP> get_return_object() {
      return Future<T, SP>{Handle::from_promise(*this), _sp};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {
      // TODO: need handle this
    }

   private:
    SP _sp;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Future(const Handle handle, SP sp)
      : _internal(handle), _sp(sp), _active(true) {}

  Future(const Future&) = delete;
  Future(Future&& other)
      : _internal(std::move(other._internal)),
        _sp(std::move(other._sp)),
        _active(std::move(other._active)) {
    other._active = false;
  }
  ~Future() {
    if (!_active)
      return;
    auto destruct = [coro = this->_internal,
                     sa = this->_sp](auto self) mutable -> void {
      if (coro.done()) {
        coro.destroy();
        sa.deleteFuncCtx();
      } else {
        sa.submit([self]() mutable { self(self); });
      }
    };
    destruct(destruct);
  }

  bool await_ready() { return _active && _internal.done(); }

  void await_suspend(std::coroutine_handle<> caller) {
    if (!_active)
      throw std::runtime_error("await on dead future");
    _sp.submit([caller, this]() {
      if (_internal.done())
        caller.resume();
      else
        await_suspend(caller);
    });
  }

  decltype(auto) await_resume() {
    if (!_active)
      throw std::runtime_error("await on dead future");
    assert(_internal.done());
    if constexpr (!std::is_void_v<T>) {
      return _internal.promise().value;
    }
  }

  std::coroutine_handle<> getCoroutineHandle() const {
    return _internal;
  }

 private:
  Handle _internal;
  SP _sp;
  bool _active;
};

}  // namespace cxxtp
