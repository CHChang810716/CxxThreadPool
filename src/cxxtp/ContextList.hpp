#pragma once
#include <cstddef>
#include <memory>
#include <cassert>
#include <mutex>
namespace cxxtp {

struct ContextHeader {
  ContextHeader *prev{nullptr};
  ContextHeader *next{nullptr};
  unsigned size{0};
  const std::uint32_t magic{0xdeadbeaf};
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
    {
      std::lock_guard<std::mutex> lock(_mux);
      _last->next = ctx;
      ctx->prev = _last;
      _last = _last->next;
    }
    return new (ctx->space) T(std::forward<T>(fctx));
  }

  template <class T>
  void remove(T *fctx) {
    static constexpr unsigned bytes = sizeof(T);
    using ContextT = Context<bytes>;
    fctx->~T();
    auto *cheader = _resolveContextHeaderFromSpaceAddr(reinterpret_cast<char*>(fctx));
    {
      std::lock_guard<std::mutex> lock(_mux);
      cheader->prev->next = cheader->next;
      if (cheader->next)
        cheader->next->prev = cheader->prev;

    }
    delete static_cast<ContextT *>(cheader);
  }

 private:
  static ContextHeader *_resolveContextHeaderFromSpaceAddr(char *space) {
    void *ctxAddr = space - sizeof(ContextHeader);
    ContextHeader *header = static_cast<ContextHeader *>(ctxAddr);
    assert(header->magic == 0xdeadbeaf);
    return header;
  }
  ContextHeader _header;
  ContextHeader *_last;
  std::mutex _mux; // TODO: lock free list
};

}  // namespace cxxtp
