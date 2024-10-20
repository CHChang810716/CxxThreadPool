#pragma once
#include <mutex>
#include <queue>
#include <vector>
#include "cxxtp/Worker.hpp"
#include "cxxtp/Task.hpp"
#include "cxxtp/TSQueue.hpp"

namespace cxxtp {
template<class T>
class Funture {};
class Scheduler {
public:
    Scheduler(unsigned numThreads);
    template<class Func>
    auto async(Func&& f) {
        Future<decltype(f(*this))> res;
        _buffer.emplace([this, res, ff = std::move(f)](){
            res.set(f(*this));
        });
    }

private:
    bool _tryAllocTasksToWorkers();
    void _schedulerLoop();
    std::vector<cxxtp::Worker*> _workers;
    std::mutex _mux;
    std::queue<Task> _buffer;
    TSQueue<Task, MAX_WORKER_TASKS> _selfTasks;
};

} // namespace cxxtp
