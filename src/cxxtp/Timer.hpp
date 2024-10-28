#pragma once

#include <chrono>
#include <coroutine>
namespace cxxtp {

template <class Scheduler, class TimePoint>
class Timer {
 public:
  Timer(Scheduler* sched, TimePoint tp)
      : _scheduler(sched), _dutime(tp) {}

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<> caller) {
    _scheduler->sched([caller, this]() {
      if (TimePoint::clock::now() < _dutime) {
        await_suspend(caller);
      } else {
        caller.resume();
      }
    });
  }
  void await_resume() {}

 private:
  Scheduler* _scheduler;
  TimePoint _dutime;
};

template <class S, class T>
Timer(S*, T) -> Timer<S, T>;

}  // namespace cxxtp