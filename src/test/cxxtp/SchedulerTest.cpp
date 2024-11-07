#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <list>
#include <random>
#include <thread>

#include "cxxtp/Future.hpp"
#include "cxxtp/Profiler.hpp"
#include "cxxtp/Scheduler.hpp"

using namespace std::chrono_literals;

TEST(Scheduler, basic_test) {
  cxxtp::Scheduler sched(4);
  using Clock = std::chrono::steady_clock;
  bool flag = false;
  auto all = sched.async([&flag](auto sched) -> cxxtp::Future<void> {
    auto start = Clock::now();
    auto f0 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f1 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f2 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f3 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });

    auto r0 = co_await f0;
    auto r1 = co_await f1;
    auto r2 = co_await f2;
    auto r3 = co_await f3;

    EXPECT_TRUE(r0 < 6s);
    EXPECT_TRUE(r0 > 5s);
    EXPECT_TRUE(r1 < 6s);
    EXPECT_TRUE(r1 > 5s);
    EXPECT_TRUE(r2 < 6s);
    EXPECT_TRUE(r2 > 5s);
    EXPECT_TRUE(r3 < 6s);
    EXPECT_TRUE(r3 > 5s);
    flag = true;
    co_return;
  });
  cxxtp::Profiler prof(2);
  sched.async(prof.make());
  sched.await(all);
  EXPECT_TRUE(flag);
  prof.print(std::cout);
}

TEST(Scheduler, yield_test) {
  cxxtp::Scheduler sched(1);
  using Clock = std::chrono::steady_clock;
  bool flag = false;
  auto all = sched.async([&flag](auto sched) -> cxxtp::Future<void> {
    auto start = Clock::now();
    auto f0 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f1 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f2 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });
    auto f3 = sched.async([&](auto api) -> cxxtp::Future<Clock::duration> {
      co_await api.suspendFor(5s);
      co_return std::chrono::steady_clock::now() - start;
    });

    auto r0 = co_await f0;
    auto r1 = co_await f1;
    auto r2 = co_await f2;
    auto r3 = co_await f3;

    EXPECT_TRUE(r0 < 6s);
    EXPECT_TRUE(r0 > 5s);
    EXPECT_TRUE(r1 < 6s);
    EXPECT_TRUE(r1 > 5s);
    EXPECT_TRUE(r2 < 6s);
    EXPECT_TRUE(r2 > 5s);
    EXPECT_TRUE(r3 < 6s);
    EXPECT_TRUE(r3 > 5s);
    flag = true;
    co_return;
  });
  sched.await(all);
  EXPECT_TRUE(flag);
}

TEST(Scheduler, block_test) {
  cxxtp::Scheduler sched(2);
  auto start = std::chrono::steady_clock::now();
  using Clock = std::chrono::steady_clock;
  bool flag = false;
  auto all = sched.async([&flag](auto sched) -> cxxtp::Future<void> {
    auto start = Clock::now();
    std::list<cxxtp::Future<Clock::duration>> futs;
    for (unsigned i = 0; i < 100; ++i) {
      auto f0 = sched.async(
          [&](auto api) -> cxxtp::Future<Clock::duration> {
            std::this_thread::sleep_for(100ms);
            co_return std::chrono::steady_clock::now() - start;
          });
      futs.emplace_back(std::move(f0));
    }
    for (auto& f : futs) {
      co_await f;
    }
    flag = true;
    co_return;
  });
  std::map<std::thread::id, unsigned> schedTimes;
  sched.async([&](auto sched) -> cxxtp::Future<void> {
    while (true) {
      auto id = std::this_thread::get_id();
      schedTimes[id]++;
      co_await sched.suspendFor(5ms);
    }
  });
  sched.await(all);
  EXPECT_TRUE(flag);
  for (auto& [k, v] : schedTimes) {
    std::cout << "thread: " << std::hex << k << std::dec
              << " times: " << v << std::endl;
  }
  auto du = std::chrono::steady_clock::now() - start;
  EXPECT_TRUE(du > 5s);
  EXPECT_TRUE(du < 6s);
}

int fib(int n) {
  if (n < 2)
    return n;
  else
    return fib(n - 1) + fib(n - 2);
}

cxxtp::Future<int> pfib(cxxtp::CoSchedApi sched, int n) {
  if (n <= 32) {
    auto res = fib(n);
    co_return res;
  }
  auto leftFut = sched.async(pfib, n - 1);
  auto rightFut = sched.async(pfib, n - 2);
  auto left = co_await leftFut;
  auto right = co_await rightFut;
  co_return left + right;
}

TEST(Scheduler, recursive_await_test) {
  constexpr int testN = 48;
  auto start = std::chrono::steady_clock::now();
  auto stRes = fib(testN);
  auto duST = std::chrono::steady_clock::now() - start;

  cxxtp::Scheduler sched(4);
  start = std::chrono::steady_clock::now();
  auto fut = sched.async(pfib, testN);
  auto mtRes = sched.await(fut);
  std::cout << mtRes << std::endl;
  auto duMT = std::chrono::steady_clock::now() - start;
  std::cout << duST.count() << std::endl;
  std::cout << duMT.count() << std::endl;
  EXPECT_EQ(mtRes, stRes);
  EXPECT_TRUE(duST * 0.25 <= duMT);
  EXPECT_TRUE(duST * 0.4 > duMT);
}
using Data = std::vector<unsigned>;
using DIter = Data::iterator;

cxxtp::Future<void> parallelSort(cxxtp::CoSchedApi sched, DIter beg,
                                 DIter end) {
  using namespace std;

  static constexpr unsigned ST_BOUND = 20 * 1000;
  auto size = end - beg;
  if (size <= 1)
    co_return;
  if (size <= ST_BOUND) {
    co_return std::sort(beg, end);
  }
  auto a = *beg;
  auto b = *(beg + (size / 2));
  auto c = *(beg + size - 1);
  auto pivot = max(min(a, b), min(max(a, b), c));
  auto left = beg;
  auto right = beg + size - 1;

  while (true) {
    while (*left <= pivot) {
      ++left;
    }
    while (pivot < *right) {
      --right;
    }
    if (left < right)
      std::swap(*left, *right);
    else
      break;
  }

  auto lfut =
      sched.async([beg, left](auto sched) -> cxxtp::Future<void> {
        return parallelSort(sched, beg, left);
      });
  auto rfut =
      sched.async([left, end](auto sched) -> cxxtp::Future<void> {
        return parallelSort(sched, left, end);
      });
  co_await lfut;
  co_await rfut;
}

TEST(Scheduler, sorting_performance_test) {
  std::random_device rd;
  std::uniform_int_distribution<unsigned> dist(0);
  unsigned dataSize = 1000 * 1000 * 10;
  Data data(dataSize);
  for (auto& d : data) {
    d = dist(rd);
  }
  auto start = std::chrono::steady_clock::now();
  auto duST = start - start;
  {
    auto stData = data;
    start = std::chrono::steady_clock::now();
    std::sort(stData.begin(), stData.end());
    duST = std::chrono::steady_clock::now() - start;
  }

  cxxtp::Scheduler sched(4);
  auto duMT = std::chrono::steady_clock::now() - start;
  {
    start = std::chrono::steady_clock::now();
    // parallelSort(sched, data.begin(), data.end());
    auto fut = sched.async(parallelSort, data.begin(), data.end());
    sched.await(fut);
    duMT = std::chrono::steady_clock::now() - start;
  }
  for (unsigned i = 1; i < data.size(); ++i) {
    EXPECT_LE(data[i - 1], data[i]);
  }
  std::cout << duST.count() << std::endl;
  std::cout << duMT.count() << std::endl;
  EXPECT_TRUE(duST * 0.25 <= duMT);
  EXPECT_TRUE(duST * 0.4 > duMT);
}