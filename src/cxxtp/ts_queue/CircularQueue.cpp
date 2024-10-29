#include "cxxtp/ts_queue/CircularQueue.hpp"

#include <cassert>
#include <thread>

namespace cxxtp::ts_queue {

void CircularQueueCheckerDebug::checkPop() {
  if (_popInited) {
    assert(_lastPop == std::this_thread::get_id());
  } else {
    _lastPop = std::this_thread::get_id();
    _popInited = true;
  }
}

void CircularQueueCheckerDebug::checkPush() {
  if (_pushInited) {
    assert(_lastPush == std::this_thread::get_id());
  } else {
    _lastPush = std::this_thread::get_id();
    _pushInited = true;
  }
}

}  // namespace cxxtp::ts_queue