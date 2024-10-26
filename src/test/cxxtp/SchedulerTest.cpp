#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>

#include "cxxtp/Scheduler.hpp"

using namespace std::chrono_literals;

// TEST(Scheduler, basic_test) {
//   cxxtp::Scheduler sched(4);
//   auto start = std::chrono::steady_clock::now();
//   auto f0 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f1 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f2 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f3 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
// 
//   auto r0 = sched.await(f0);
//   auto r1 = sched.await(f1);
//   auto r2 = sched.await(f2);
//   auto r3 = sched.await(f3);
// 
//   EXPECT_TRUE(r0 < 6s);
//   EXPECT_TRUE(r0 > 5s);
//   EXPECT_TRUE(r1 < 6s);
//   EXPECT_TRUE(r1 > 5s);
//   EXPECT_TRUE(r2 < 6s);
//   EXPECT_TRUE(r2 > 5s);
//   EXPECT_TRUE(r3 < 6s);
//   EXPECT_TRUE(r3 > 5s);
// }
// 
// TEST(Scheduler, yield_test) {
//   cxxtp::Scheduler sched(1);
//   auto start = std::chrono::steady_clock::now();
//   auto f0 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f1 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f2 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f3 = sched.async([&](auto &api) {
//     api.yield_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
// 
//   auto r0 = sched.await(f0);
//   auto r1 = sched.await(f1);
//   auto r2 = sched.await(f2);
//   auto r3 = sched.await(f3);
// 
//   EXPECT_TRUE(r0 < 6s);
//   EXPECT_TRUE(r0 > 5s);
//   EXPECT_TRUE(r1 < 6s);
//   EXPECT_TRUE(r1 > 5s);
//   EXPECT_TRUE(r2 < 6s);
//   EXPECT_TRUE(r2 > 5s);
//   EXPECT_TRUE(r3 < 6s);
//   EXPECT_TRUE(r3 > 5s);
// }
// 
// TEST(Scheduler, block_test) {
//   cxxtp::Scheduler sched(2);
//   auto start = std::chrono::steady_clock::now();
//   auto f0 = sched.async([&](auto &api) {
//     std::this_thread::sleep_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f1 = sched.async([&](auto &api) {
//     std::this_thread::sleep_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f2 = sched.async([&](auto &api) {
//     std::this_thread::sleep_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
//   auto f3 = sched.async([&](auto &api) {
//     std::this_thread::sleep_for(5s);
//     return std::chrono::steady_clock::now() - start;
//   });
// 
//   auto r0 = sched.await(f0);
//   auto r1 = sched.await(f1);
//   auto r2 = sched.await(f2);
//   auto r3 = sched.await(f3);
// 
//   auto du = std::chrono::steady_clock::now() - start;
//   EXPECT_TRUE(du > 10s);
//   EXPECT_TRUE(du < 12s);
// }
// 
// int fib(int n) {
//   if (n < 2)
//     return n;
//   else
//     return fib(n - 1) + fib(n - 2);
// }
// 
// int pfib(cxxtp::Scheduler &sched, int n) {
//   if (n <= 32) return fib(n);
//   auto leftFut =
//       sched.async([&](auto &api) { return pfib(sched, n - 1); });
//   auto rightFut =
//       sched.async([&](auto &api) { return pfib(sched, n - 2); });
//   return sched.await(leftFut) + sched.await(rightFut);
// }
// 
// TEST(Scheduler, recursive_await_test) {
//   auto start = std::chrono::steady_clock::now();
//   auto stRes = fib(48);
//   auto duST = std::chrono::steady_clock::now() - start;
// 
//   cxxtp::Scheduler sched(4);
//   start = std::chrono::steady_clock::now();
//   auto mtRes = pfib(sched, 48);
//   auto duMT = std::chrono::steady_clock::now() - start;
//   EXPECT_EQ(mtRes, stRes);
//   EXPECT_TRUE(duST * 0.25 <= duMT);
//   EXPECT_TRUE(duST * 0.3 > duMT);
// }
// 
// template <class RIter>
// void parallelSort(cxxtp::Scheduler &sched, RIter beg, RIter end) {
//   using namespace std;
// 
//   static constexpr unsigned ST_BOUND = 20 * 1000;
//   auto size = end - beg;
//   if (size <= 1) return;
//   if (size <= ST_BOUND) {
//     return std::sort(beg, end);
//   }
//   auto a = *beg;
//   auto b = *(beg + (size / 2));
//   auto c = *(beg + size - 1);
//   auto pivot = max(min(a, b), min(max(a, b), c));
//   auto left = beg;
//   auto right = beg + size - 1;
// 
//   while (true) {
//     while (*left <= pivot) {
//       ++left;
//     }
//     while (pivot < *right) {
//       --right;
//     }
//     if (left < right)
//       std::swap(*left, *right);
//     else
//       break;
//   }
// 
//   std::future<void> lfut = sched.async(
//       [beg, left](auto &sched) { return parallelSort(sched, beg, left); });
//   parallelSort(sched, left, end);
//   sched.await(lfut);
// }
// 
// TEST(Scheduler, sorting_performance_test) {
//   std::random_device rd;
//   std::uniform_int_distribution<unsigned> dist(0);
//   unsigned dataSize = 1000 * 1000 * 10;
//   std::vector<unsigned> data(dataSize);
//   for (auto &d : data) {
//     d = dist(rd);
//   }
//   auto start = std::chrono::steady_clock::now();
//   auto duST = start - start;
//   {
//     auto stData = data;
//     start = std::chrono::steady_clock::now();
//     std::sort(stData.begin(), stData.end());
//     duST = std::chrono::steady_clock::now() - start;
//   }
// 
//   cxxtp::Scheduler sched(4);
//   auto duMT = std::chrono::steady_clock::now() - start;
//   {
//     start = std::chrono::steady_clock::now();
//     parallelSort(sched, data.begin(), data.end());
//     duMT = std::chrono::steady_clock::now() - start;
//   }
//   for (unsigned i = 1; i < data.size(); ++i) {
//     EXPECT_LE(data[i - 1], data[i]);
//   }
//   std::cout << duST.count() << std::endl;
//   std::cout << duMT.count() << std::endl;
//   // EXPECT_TRUE(duST * 0.25 <= duMT);
//   // EXPECT_TRUE(duST * 0.3 > duMT);
// }