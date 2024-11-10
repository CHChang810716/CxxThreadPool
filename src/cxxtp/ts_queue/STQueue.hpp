#pragma once
#include <mutex>
#include <optional>
#include <queue>
#include "cxxtp/ts_queue/Status.hpp"

namespace cxxtp::ts_queue {

template <class Elem, unsigned maxSize>
class STQueue {
 public:
  TransRes<Elem> tryPop();           // has value -> success
  TransRes<Elem> tryPush(Elem&& o);  // has value -> failed
  TransStatus tryPush(Elem& o);
  unsigned size() const { return _size; }

 private:
  unsigned _size {0};
  std::queue<Elem> _data;
};

template <class Elem, unsigned maxSize>
TransRes<Elem> STQueue<Elem, maxSize>::tryPop() {
  if (_data.empty())
    return TS_EMPTY;
  Elem res = std::move(_data.front());
  _data.pop();
  --_size;
  return {std::move(res), TS_DONE};
}

template <class Elem, unsigned maxSize>
TransRes<Elem> STQueue<Elem, maxSize>::tryPush(Elem&& obj) {
  _data.push(std::move(obj));
  ++_size;
  return {std::nullopt, TS_DONE};
}

template <class Elem, unsigned maxSize>
TransStatus STQueue<Elem, maxSize>::tryPush(Elem& obj) {
  _data.push(obj);
  ++_size;
  return TS_DONE;
}

}  // namespace cxxtp::ts_queue
