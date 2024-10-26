#pragma once
#include <coroutine>
#include <functional>
#include <future>
#include <cassert>

namespace cxxtp {
class Scheduler;

template <class T>
struct PromiseStorage {
  T value;
};

template <>
struct PromiseStorage<void> {};

template<class T>
concept SchedulerProxy = requires(T a, std::coroutine_handle<> co) {
  { a.deleteFuncCtx() };
  { a.sched(co) };
};

template <class T, SchedulerProxy SchedulerAgent>
class Future {
 public:
  struct promise_type : public PromiseStorage<T> {
    // TODO: operator new for memory efficiency
    // constructor forwarding https://godbolt.org/z/T59dcebPM
    promise_type(SchedulerAgent &sa) : _schedulerAgent(&sa) {}

    Future<T, SchedulerAgent> get_return_object() {
      return Future<T, SchedulerAgent>{Handle::from_promise(*this),
                                       _schedulerAgent};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() requires(std::is_void_v<T>) {}
    void return_value(T &&v) requires(!std::is_void_v<T>) {
      PromiseStorage<T>::value = std::forward<T>(v);
    }
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

  bool await_ready() {
    return _internal.done();
  }

  void await_suspend(std::coroutine_handle<> caller) {
    _schedulerAgent->sched(caller);
  }

  T &await_resume() {
    assert(_internal.done());
    if constexpr(!std::is_void_v<T>) {
      return _internal.promise().value;
    }
  }
 private:
  Handle _internal;
  SchedulerAgent *_schedulerAgent;
};

}  // namespace cxxtp
