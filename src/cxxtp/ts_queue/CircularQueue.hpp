#pragma once
#include <atomic>
#include <optional>
#include <queue>
#include <thread>

namespace cxxtp::ts_queue {

class CircularQueueCheckerDebug {
 public:
  void checkPush();
  void checkPop();

 private:
  bool _pushInited{false};
  bool _popInited{false};
  std::thread::id _lastPush;
  std::thread::id _lastPop;
};

class CircularQueueCheckerRelease {
 public:
  inline void checkPush() {}
  inline void checkPop() {}
};

#ifdef NDEBUG
using CircularQueueChecker = CircularQueueCheckerRelease;
#else
using CircularQueueChecker = CircularQueueCheckerDebug;
#endif

template <class Elem, unsigned maxSize>
class CircularQueue : private CircularQueueChecker {
 public:
  std::optional<Elem> tryPop();           // has value -> success
  std::optional<Elem> tryPush(Elem&& o);  // has value -> failed
  bool tryPush(Elem& o);
  unsigned size() const;

 private:
  static constexpr unsigned arrSize = maxSize + 1;
  static unsigned _size(unsigned b, unsigned f);
  static unsigned _next(unsigned i);
  std::atomic<unsigned> _front{0};
  std::atomic<unsigned> _back{0};
  Elem _data[arrSize];
};

template <class Elem, unsigned maxSize>
unsigned CircularQueue<Elem, maxSize>::_next(unsigned i) {
  return (i + 1) % arrSize;
}
// always move index at operation end
template <class Elem, unsigned maxSize>
std::optional<Elem> CircularQueue<Elem, maxSize>::tryPop() {
  // pop front push back
  checkPop();
  auto f = _front.load(std::memory_order_acquire);
  auto b = _back.load(std::memory_order_acquire);
  if (f == b)
    return std::nullopt;
  Elem res = std::move(_data[f]);
  _front.store(_next(f), std::memory_order_release);
  return res;
}

template <class Elem, unsigned maxSize>
std::optional<Elem> CircularQueue<Elem, maxSize>::tryPush(
    Elem&& obj) {
  // pop front push back
  checkPush();
  auto f = _front.load(std::memory_order_acquire);
  auto b = _back.load(std::memory_order_acquire);
  auto nb = _next(b);
  if (nb == f)
    return obj;
  _data[b] = std::move(obj);
  _back.store(nb, std::memory_order_release);
  return std::nullopt;
}
template <class Elem, unsigned maxSize>
bool CircularQueue<Elem, maxSize>::tryPush(Elem& obj) {
  checkPush();
  auto copy = [](Elem& o) -> Elem&& {
    return o;
  };
  auto tmp = tryPush(copy(obj));
  return !tmp.has_value();
}
// maxSize = 3
// arrSize = 4
// f
// b
// ----
// b = 0
// f = 0
// size = b - f = 0
//
// fb
// ----
// b = 1
// f = 0
// size = b - f = 1
//
// b  f
// ----
// b = 0
// f = 3
// size = b + arrSize - f = 1
//
//  b f
// ----
// b = 1
// f = 3
// size = b + arrSize - f = 2
//
//   bf
// ----
// b = 2
// f = 3
// size = b + arrSize - f = 3 (full)

template <class Elem, unsigned maxSize>
unsigned CircularQueue<Elem, maxSize>::_size(unsigned b, unsigned f) {
  if (b >= f) {
    return b - f;
  } else {
    return b + arrSize + 1 - f;
  }
}

template <class Elem, unsigned maxSize>
unsigned CircularQueue<Elem, maxSize>::size() const {
  auto b = _back.load(std::memory_order_relaxed);
  auto f = _front.load(std::memory_order_relaxed);
  return _size(b, f);
}

}  // namespace cxxtp::ts_queue
