#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <list>
#include <random>
#include <thread>
#include <vector>
#include "cxxtp/ts_queue/Status.hpp"
#include "cxxtp/ts_queue/VersionQueue.hpp"

template <class Queue>
void basicTest(Queue& q) {
  auto tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_EMPTY);
  q.tryPush(1);
  q.tryPush(2);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  EXPECT_EQ(tmp.value(), 1);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  EXPECT_EQ(tmp.value(), 2);
  q.tryPush(3);
  q.tryPush(4);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  EXPECT_EQ(tmp.value(), 3);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  EXPECT_EQ(tmp.value(), 4);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_EMPTY);
  for (int i = 0; i < 5; ++i) {
    q.tryPush(i);
  }
  tmp = q.tryPush(5);
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_FULL);
}

template <class Queue>
void threadTest(int consumer, int producer, unsigned testSize) {
  Queue q;
  std::atomic<int>* data = new std::atomic<int>[testSize];
  for (int i = 0; i < testSize; ++i) {
    data[i] = 0;
  }
  std::vector<std::thread> conTs(consumer);
  std::vector<std::thread> proTs(producer);
  std::atomic<bool> testPop = true;
  for (auto& t : conTs) {
    t = std::thread([&] {
      while (testPop.load()) {
        auto tmp = q.tryPop();
        if (tmp.status == cxxtp::ts_queue::TS_DONE) {
          auto& d = data[tmp.value()];
          d.fetch_add(1, std::memory_order_consume);
        }
      }
    });
  }
  for (auto& t : proTs) {
    t = std::thread([&] {
      for (int i = 0; i < testSize; ++i) {
        while(q.tryPush(i) != cxxtp::ts_queue::TS_DONE) {
          std::this_thread::yield();
        }
      }
    });
  }
  for (auto& t : proTs) {
    t.join();
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  testPop.store(false);
  for (auto& t : conTs) {
    t.join();
  }
  for (int i = 0; i < testSize; ++i) {
    EXPECT_EQ(data[i].load(std::memory_order_relaxed), producer);
  }
  delete [] data;
}
TEST(VersionQueue, basic_test) {
  cxxtp::ts_queue::VersionQueue<int, 5> q;
  basicTest(q);
}

TEST(VersionQueue, thread_test) {
  using Q = cxxtp::ts_queue::VersionQueue<int, 5>;
  threadTest<Q>(8, 1, 3);
  threadTest<Q>(8, 1, 1000);
}