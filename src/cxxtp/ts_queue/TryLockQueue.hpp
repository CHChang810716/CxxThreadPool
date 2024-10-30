#pragma once
#include <mutex>
#include <optional>
#include <queue>
#include "cxxtp/ts_queue/Status.hpp"

namespace cxxtp::ts_queue {

template <class Elem, unsigned maxSize>
class TryLockQueue {
 public:
  TransRes<Elem> tryPop();           // has value -> success
  TransRes<Elem> tryPush(Elem&& o);  // has value -> failed
  TransStatus tryPush(Elem& o);
  unsigned size() const { return _size; }

 private:
  std::mutex _mux;
  std::queue<Elem> _data;
  unsigned _size{0};
};

template <class Elem, unsigned maxSize>
TransRes<Elem> TryLockQueue<Elem, maxSize>::tryPop() {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock())
    return TS_RACE;
  if (_data.empty())
    return TS_EMPTY;
  Elem res = std::move(_data.front());
  _data.pop();
  --_size;
  return {std::move(res), TS_DONE};
}

template <class Elem, unsigned maxSize>
TransRes<Elem> TryLockQueue<Elem, maxSize>::tryPush(Elem&& obj) {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock())
    return {std::move(obj), TS_RACE};
  if (_data.size() >= maxSize)
    return {std::move(obj), TS_FULL};
  ++_size;
  _data.push(std::move(obj));
  return {std::nullopt, TS_DONE};
}

template <class Elem, unsigned maxSize>
TransStatus TryLockQueue<Elem, maxSize>::tryPush(Elem& obj) {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock())
    return TS_RACE;
  if (_data.size() >= maxSize)
    return TS_FULL;
  ++_size;
  _data.push(obj);
  return TS_DONE;
}

}  // namespace cxxtp::ts_queue
