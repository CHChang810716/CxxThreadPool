#include "cxxtp/ts_queue/VersionQueue.hpp"

#include <cassert>
#include <thread>

namespace cxxtp::ts_queue {

void VersionQueueCheckerDebug::checkPush() {
  if (_pushInited) {
    assert(_lastPush == std::this_thread::get_id());
  } else {
    _lastPush = std::this_thread::get_id();
    _pushInited = true;
  }
}

}  // namespace cxxtp::ts_queue