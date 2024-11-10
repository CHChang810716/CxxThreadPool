#pragma once
#include <mutex>
#include <queue>
namespace cxxtp::ts_queue {

template <class T>
class LockQueue {
 public:
  void push(T&& o) {
    std::lock_guard<std::mutex> lock(_mux);
    _data.push(std::move(o));
    _size ++;
  }

  T pop() {
    std::lock_guard<std::mutex> lock(_mux);
    T o = std::move(_data.front());
    _data.pop();
    _size --;
    return o;
  }

  std::size_t size() const {
    return _size;
  }

  std::queue<T> takeAll() {
    std::lock_guard<std::mutex> lock(_mux);
    std::queue<T> tmp;
    _data.swap(tmp);
    _size = 0;
    return std::move(tmp);
  }

 private:
  std::mutex _mux;
  std::queue<T> _data;
  std::size_t _size {0};
};

}  // namespace cxxtp::ts_queue