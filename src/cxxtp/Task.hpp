#pragma once
#include <functional>
#include <coroutine>
#include <future>

namespace cxxtp {
class Scheduler;

template<class T, class FuncCtx>
class Future : public std::future<T> {

};

// Task is a wrapper of coroutine handle
// using Task = std::function<void(void)>;
template<class Ret>
class Task {
public:
  struct promise_type {};
  Task(std::coroutine_handle<promise_type>&& handle)
  {}
  
};

} // namespace cxxtp
