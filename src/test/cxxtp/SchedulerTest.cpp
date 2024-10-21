#include "cxxtp/Scheduler.hpp"
#include <chrono>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(Scheduler, basic_test) {
  cxxtp::Scheduler sched(4);
  auto start = std::chrono::steady_clock::now();
  auto f0 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f1 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f2 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f3 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });

  auto r0 = sched.await(f0);
  auto r1 = sched.await(f1);
  auto r2 = sched.await(f2);
  auto r3 = sched.await(f3);

  EXPECT_TRUE(r0 < 6s);
  EXPECT_TRUE(r0 > 5s);
  EXPECT_TRUE(r1 < 6s);
  EXPECT_TRUE(r1 > 5s);
  EXPECT_TRUE(r2 < 6s);
  EXPECT_TRUE(r2 > 5s);
  EXPECT_TRUE(r3 < 6s);
  EXPECT_TRUE(r3 > 5s);
}


TEST(Scheduler, yield_test) {
  cxxtp::Scheduler sched(1);
  auto start = std::chrono::steady_clock::now();
  auto f0 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f1 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f2 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f3 = sched.async([&](auto &api) {
    api.yield_for(5s);
    return std::chrono::steady_clock::now() - start;
  });

  auto r0 = sched.await(f0);
  auto r1 = sched.await(f1);
  auto r2 = sched.await(f2);
  auto r3 = sched.await(f3);

  EXPECT_TRUE(r0 < 6s);
  EXPECT_TRUE(r0 > 5s);
  EXPECT_TRUE(r1 < 6s);
  EXPECT_TRUE(r1 > 5s);
  EXPECT_TRUE(r2 < 6s);
  EXPECT_TRUE(r2 > 5s);
  EXPECT_TRUE(r3 < 6s);
  EXPECT_TRUE(r3 > 5s);
}

TEST(Scheduler, block_test) {
  cxxtp::Scheduler sched(2);
  auto start = std::chrono::steady_clock::now();
  auto f0 = sched.async([&](auto &api) {
    std::this_thread::sleep_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f1 = sched.async([&](auto &api) {
    std::this_thread::sleep_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f2 = sched.async([&](auto &api) {
    std::this_thread::sleep_for(5s);
    return std::chrono::steady_clock::now() - start;
  });
  auto f3 = sched.async([&](auto &api) {
    std::this_thread::sleep_for(5s);
    return std::chrono::steady_clock::now() - start;
  });

  auto r0 = sched.await(f0);
  auto r1 = sched.await(f1);
  auto r2 = sched.await(f2);
  auto r3 = sched.await(f3);

  auto du = std::chrono::steady_clock::now() - start;
  EXPECT_TRUE(du > 10s);
  EXPECT_TRUE(du < 12s);
}

int fib(int n) {
  if( n < 2 ) return n;
  else return fib(n - 1) + fib( n - 2 );
}

int pfib(cxxtp::Scheduler& sched, int n) {
  if (n <= 32)
    return fib(n);
  auto leftFut = sched.async([&](auto& api){
    return pfib(sched, n - 1);
  });
  auto rightFut = sched.async([&](auto& api){
    return pfib(sched, n - 2);
  });
  return sched.await(leftFut) + sched.await(rightFut);
}
TEST(Scheduler, recursive_await_test) {
  auto start = std::chrono::steady_clock::now();
  auto stRes = fib(48);
  auto duST = std::chrono::steady_clock::now() - start;

  cxxtp::Scheduler sched(4);
  start = std::chrono::steady_clock::now();
  auto mtRes = pfib(sched, 48);
  auto duMT = std::chrono::steady_clock::now() - start;
  EXPECT_EQ(mtRes, stRes);
  EXPECT_TRUE(duST * 0.25 <= duMT);
  EXPECT_TRUE(duST * 0.3 > duMT);
}