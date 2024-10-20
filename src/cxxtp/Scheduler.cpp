#include "cxxtp/Scheduler.hpp"

namespace cxxtp {

Scheduler::Scheduler(unsigned numThreads)
    : _workers()
    , _mux()
    , _buffer()
    , _selfTasks()
    {

    }

    
} // namespace cxxtp
