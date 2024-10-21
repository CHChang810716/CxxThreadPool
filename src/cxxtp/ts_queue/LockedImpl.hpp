#pragma once
#include <mutex>
#include <optional>
#include <queue>

namespace cxxtp::ts_queue {

template <class Elem, unsigned maxSize>
class LockedImpl {
 public:
  std::optional<Elem> tryPop();           // has value -> success
  std::optional<Elem> tryPush(Elem &&o);  // has value -> failed
  bool tryPush(Elem &o);
  unsigned size() const { return _data.size(); }  // TODO: need mutx?

 private:
  std::mutex _mux;
  std::queue<Elem> _data;
};

template <class Elem, unsigned maxSize>
std::optional<Elem> LockedImpl<Elem, maxSize>::tryPop() {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock()) return std::nullopt;
  if (_data.empty()) return std::nullopt;
  Elem res = std::move(_data.front());
  _data.pop();
  return {std::move(res)};
}

template <class Elem, unsigned maxSize>
std::optional<Elem> LockedImpl<Elem, maxSize>::tryPush(Elem &&obj) {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock()) return {std::move(obj)};
  if (_data.size() >= maxSize) return {std::move(obj)};
  _data.push(std::move(obj));
  return std::nullopt;
}

template <class Elem, unsigned maxSize>
bool LockedImpl<Elem, maxSize>::tryPush(Elem &obj) {
  std::unique_lock<std::mutex> lock(_mux, std::try_to_lock);
  if (!lock.owns_lock()) return false;
  if (_data.size() >= maxSize) return false;
  _data.push(obj);
  return true;
}

}  // namespace cxxtp::ts_queue
