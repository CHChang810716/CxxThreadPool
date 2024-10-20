#pragma once
#include <cxxtp/ts_queue/LockedImpl.hpp>

namespace cxxtp {

template <class Elem, unsigned maxSize>
class TSQueue : public ts_queue::LockedImpl<Elem, maxSize> {};

} // namespace cxxtp