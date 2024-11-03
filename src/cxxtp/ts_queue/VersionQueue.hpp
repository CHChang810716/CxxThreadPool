#pragma once
#include <atomic>
#include <cassert>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "cxxtp/ts_queue/Status.hpp"

namespace cxxtp::ts_queue {
class VersionQueueCheckerDebug {
 public:
  void checkPush();

 private:
  bool _pushInited{false};
  std::thread::id _lastPush;
};

class VersionQueueCheckerRelease {
 public:
  inline void checkPush() {}
};

#ifdef NDEBUG
using VersionQueueChecker = VersionQueueCheckerRelease;
#else
using VersionQueueChecker = VersionQueueCheckerDebug;
#endif
template <class T>
struct VersionBlock {
  T obj;
  std::atomic<unsigned> version{0};
};

template <class Elem, unsigned maxSize>
class VersionQueue : public VersionQueueChecker {
  static_assert(
      maxSize < 65536,
      "maxSize too large, the integer distance might overflow");
  using Block = VersionBlock<Elem>;

 public:
  TransRes<Elem> tryPop();           // has value -> success
  TransRes<Elem> tryPush(Elem&& o);  // has value -> failed
  TransStatus tryPush(Elem& o);
  unsigned size() const { return _size; }

 private:
  unsigned _next(unsigned i) { return (i + 1) % maxSize; }
  unsigned _nextVer(unsigned i) { return (i + 1) == 0 ? 1 : (i + 1); }
  int _intDist(unsigned l, unsigned r) { return int(r - l); }
  struct ConsumerApi {
    unsigned readIdx{0};
  };

  std::vector<Block> _data{maxSize};
  unsigned _size{0};
  unsigned _writeIdx{0};
  unsigned _curVersion{1};
};

template <class Elem, unsigned maxSize>
TransRes<Elem> VersionQueue<Elem, maxSize>::tryPop() {
  static constexpr unsigned retryTimes = 5;
  static thread_local ConsumerApi api;
  for (int i = 0; i < retryTimes; ++i) {
    int d = _intDist(api.readIdx, _writeIdx);
    assert(d >= 0 && "queue in bug state, stop");
    if (d == 0)
      return {std::nullopt, TS_EMPTY};
    auto& b = _data[api.readIdx];
    api.readIdx = _next(api.readIdx);
    // If version is not 0, update the version and read the data.
    // And because there might me multiple consumers, we need to take care the memory order of version.
    auto bv = b.version.load(std::memory_order_acquire); // read version
    if (bv != 0) {
      // CAS
      if (b.version.compare_exchange_weak(bv, 0, std::memory_order_acq_rel)) {
        // Now we have the block data, read the data and return.
        return {std::move(b.obj), TS_DONE};
      }
    }
  }
  return {std::nullopt, TS_RACE};
}

template <class Elem, unsigned maxSize>
TransRes<Elem> VersionQueue<Elem, maxSize>::tryPush(Elem&& o) {
  checkPush();
  TransRes<Elem> res;
  auto& b = _data[_writeIdx];
  // write data if version == 0
  if (b.version == 0) {
    b.obj = std::move(o);
    b.version.store(_curVersion, std::memory_order_acquire);
    _curVersion = _nextVer(_curVersion);
    res.status = TS_DONE;
  } else {
    res = {std::move(o), TS_FULL};
  }
  _writeIdx = _next(_writeIdx);
  return res;
}

template <class Elem, unsigned maxSize>
TransStatus VersionQueue<Elem, maxSize>::tryPush(Elem& o) {
  auto copy = [](auto& x) {
    return x;
  };
  auto tmp = tryPush(copy(o));
  return tmp.status;
}

}  // namespace cxxtp::ts_queue
