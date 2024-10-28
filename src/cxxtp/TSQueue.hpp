#pragma once
#include <cxxtp/ts_queue/TryLockQueue.hpp>

namespace cxxtp {

template <class Elem, unsigned maxSize>
class TSQueue : public ts_queue::TryLockQueue<Elem, maxSize> {};

} // namespace cxxtp