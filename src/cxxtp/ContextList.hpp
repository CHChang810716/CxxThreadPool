#pragma once
#include <cstddef>
#include <memory>

namespace cxxtp {

struct ContextHeader {
  ContextHeader *prev{nullptr};
  ContextHeader *next{nullptr};
  unsigned size{0};
};

template <unsigned bytes>
struct Context : public ContextHeader {
  Context() : ContextHeader{nullptr, nullptr, bytes} {}
  std::byte space[bytes];
};

class ContextList {
 public:
  ContextList() : _header{nullptr, nullptr, 0}, _last(nullptr) {
    _last = &_header;
  }

  template <class T>
  T *append(T &&fctx) {
    static constexpr unsigned bytes = sizeof(T);
    using ContextT = Context<bytes>;
    ContextT *ctx = new ContextT();
    _last->next = ctx;
    ctx->prev = _last;
    _last = _last->next;
    return new (ctx->space) T(std::forward<T>(fctx));
  }

  template <class T>
  static void remove(T *fctx) {
    static constexpr unsigned bytes = sizeof(T);
    using ContextT = Context<bytes>;
    fctx->~T();
    auto *cheader = _resolveContextHeaderFromSpaceAddr(fctx);
    cheader->prev->next = cheader->next;
    cheader->next->prev = cheader->prev;
    delete static_cast<ContextT*>(cheader);
  }

 private:
  static ContextHeader *_resolveContextHeaderFromSpaceAddr(char *space) {
    void *ctxAddr = space - offsetof(Context<0>, space);
    return static_cast<ContextHeader *>(ctxAddr);
  }
  ContextHeader _header;
  ContextHeader *_last;
};

}  // namespace cxxtp
