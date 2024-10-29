#pragma once
#include <cxxtp/ts_queue/TryLockQueue.hpp>
#include "cxxtp/ts_queue/CircularQueue.hpp"

namespace cxxtp {

// template <class Elem, unsigned maxSize>
// class TSQueue : public ts_queue::TryLockQueue<Elem, maxSize> {};

template <class Elem, unsigned maxSize>
class TSQueue : public ts_queue::CircularQueue<Elem, maxSize> {};

} // namespace cxxtp