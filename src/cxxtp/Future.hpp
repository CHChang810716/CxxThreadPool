#pragma once
#include <coroutine>
#include <functional>
#include <future>

namespace cxxtp {
class Scheduler;

template <class T, class SchedulerAgent>
class Future : public std::future<T> {
 public:
  struct promise_type {
    // TODO: operator new for memory efficiency
    promise_type(SchedulerAgent& sa)
    :_schedulerAgent(&sa)
    {}

    Future<T, SchedulerAgent> get_return_object() {
      return Future<T, SchedulerAgent>{Handle::from_promise(*this), _schedulerAgent};
    }
    SchedulerAgent *_schedulerAgent;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Future(const Handle handle, SchedulerAgent *sa)
  : _internal(handle)
  , _schedulerAgent(sa)
  {}
  // https://godbolt.org/z/T59dcebPM
  Future() = delete;
  Future(const Future&) = delete;
  Future(Future&&) = default;
  ~Future() {
    using FuncCtx = typename SchedulerAgent::FuncCtx;
    if (_internal)
      _internal.destroy();
    if (_schedulerAgent)
      _schedulerAgent->;
  }

private:
  Handle _internal;
  SchedulerAgent *_schedulerAgent;
};

}  // namespace cxxtp
