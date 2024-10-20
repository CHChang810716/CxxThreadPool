#pragma once
#include <thread>
#include <functional>
#include <atomic>
#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"

namespace cxxtp {

constexpr unsigned MAX_WORKER_TASKS = 512;
class Scheduler;

// task life cycle
// 1. pop from worker
// 2. run by worker
class Worker {
public:

    Worker(Scheduler& sched);
    bool trySubmit(Task& t);
private:
    void _workerLoop();
    std::atomic<bool> _enabled;
    std::thread _t;
    TSQueue<Task, MAX_WORKER_TASKS> _queue;
    Scheduler* _scheduler;
};
    
} // namespace cxxtp
