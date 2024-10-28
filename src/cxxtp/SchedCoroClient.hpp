#pragma once

#include <functional>
#include <utility>

#include "cxxtp/ContextList.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

template <class Server>
class SchedCoroClient {
 public:
  SchedCoroClient(Server* sched)
      : _server(sched), _deleter([]() {}) {}

  SchedCoroClient(Server* sched, std::function<void(void)>&& fctxDel)
      : _server(sched), _deleter(std::move(fctxDel)) {}

  template <class Fctx>
  SchedCoroClient(Server* sched, Fctx* ptr)
      : _server(sched),
        _deleter([ptr, sched]() { sched->deleteFuncCtx(ptr); }) {}

  void deleteFuncCtx() { _deleter(); }

  template <class Du>
  auto suspendFor(Du&& du) {
    return _server->suspendfor(std::forward<Du>(du));
  }

  template <class... Args>
  decltype(auto) async(Args&&... args) {
    return _server->async(std::forward<Args>(args)...);
  }

  void submit(Task&& t) { _server->submit(std::move(t)); }
 private:
  Server* _server{nullptr};
  std::function<void(void)> _deleter{nullptr};
};
template <class S, class... Args>
SchedCoroClient(S* s, Args&&... args) -> SchedCoroClient<S>;
}  // namespace cxxtp