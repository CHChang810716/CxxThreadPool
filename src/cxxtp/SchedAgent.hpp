#pragma once

#include <functional>
#include <utility>

#include "cxxtp/ContextList.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

template <class Scheduler>
class SchedAgent {
 public:
  SchedAgent(Scheduler* sched, std::function<void(void)>&& fctxDel)
      : _server(sched), _deleter(std::move(fctxDel)) {}
  template <class Fctx>
  SchedAgent(Scheduler* sched, Fctx* ptr)
      : _server(sched), _deleter([ptr]() { ContextList::remove(ptr); }) {}
  void deleteFuncCtx() { 
    _deleter(); 
  }
  void sched(Task&& task) { _server->sched(std::move(task)); }
  template <class Du>
  auto sleep_for(Du&& du) {
    return _server->sleep_for(std::forward<Du>(du));
  }
  template <class F>
  decltype(auto) async(F&& f) {
    return _server->async(std::forward<F>(f));
  }

 private:
  Scheduler* _server;
  std::function<void(void)> _deleter;
};
template <class S, class... Args>
SchedAgent(S* s, Args&&... args) -> SchedAgent<S>;
}  // namespace cxxtp