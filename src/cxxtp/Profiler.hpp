#pragma once
#include <chrono>
#include <coroutine>
#include <iostream>
#include <map>
#include <mutex>
#include <ostream>
#include <thread>

namespace cxxtp {

#ifdef TP_PROF
class Profiler {
  struct WorkerStats {
    unsigned runTasks = 0;
    unsigned stealedTasks = 0;
  };

 public:
  Profiler() = default;
  ~Profiler() = default;
  inline static void workerRunTask(bool steal = false) {
    get()._workerRunTask(steal);
  }
  inline static void print(std::ostream& os) { get()._print(os); }

 private:
  static bool& isInit() {
    static thread_local bool inited = false;
    return inited;
  }
  inline void _workerRunTask(bool steal) {
    if (!isInit()) {
      std::lock_guard<std::mutex> lock(_mux);
      _workerStats[std::this_thread::get_id()] = {};
      isInit() = true;
    }
    _workerStats[std::this_thread::get_id()].runTasks++;
    if (steal) {
      _workerStats[std::this_thread::get_id()].stealedTasks++;
    }
  }
  inline void _print(std::ostream& os) {
    for (auto& kv : _workerStats) {
      os << "thread :" << std::hex << kv.first << std::dec
         << std::endl;
      os << "  tasks run:" << kv.second.runTasks << std::endl;
      os << "  stealed tasks run:" << kv.second.stealedTasks
         << std::endl;
    }
    _workerStats.clear();
  }
  static Profiler& get() {
    static Profiler p;
    return p;
  }
  std::mutex _mux;
  std::map<std::thread::id, WorkerStats> _workerStats;
};
#else
class Profiler {
 public:
  inline static void workerRunTask(bool steal = false) {}
  inline static void print(std::ostream& os) {}
};
#endif
}  // namespace cxxtp
