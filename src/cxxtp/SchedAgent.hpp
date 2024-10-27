#pragma once

#include <functional>
#include <utility>

#include "cxxtp/ContextList.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

template <class Scheduler>
class SchedAgent {
 public:
  SchedAgent(Scheduler* sched) : _server(sched), _deleter([]() {}) {}

  SchedAgent(Scheduler* sched, std::function<void(void)>&& fctxDel)
      : _server(sched), _deleter(std::move(fctxDel)) {}

  template <class Fctx>
  SchedAgent(Scheduler* sched, Fctx* ptr)
      : _server(sched), _deleter([ptr]() { ContextList::remove(ptr); }) {}

  void deleteFuncCtx() { _deleter(); }

  void sched(Task&& task) { _server->sched(std::move(task)); }
  void suspend(Task&& task) { _server->suspend(std::move(task)); }

  template <class Du>
  auto sleep_for(Du&& du) {
    return _server->sleep_for(std::forward<Du>(du));
  }

  template <class... Args>
  decltype(auto) async(Args&&... args) {
    return _server->async(std::forward<Args>(args)...);
  }

 private:
  Scheduler* _server {nullptr};
  std::function<void(void)> _deleter {nullptr};
};
template <class S, class... Args>
SchedAgent(S* s, Args&&... args) -> SchedAgent<S>;
}  // namespace cxxtp