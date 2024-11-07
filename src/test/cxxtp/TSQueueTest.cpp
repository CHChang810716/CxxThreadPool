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
  std::vector<int> v;
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  std::sort(v.begin(), v.end());
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  v.clear();
  q.tryPush(3);
  q.tryPush(4);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  std::sort(v.begin(), v.end());
  EXPECT_EQ(v[0], 3);
  EXPECT_EQ(v[1], 4);
  tmp = q.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_EMPTY);
  for (int i = 0; i < 5; ++i) {
    q.tryPush(i);
  }
  tmp = q.tryPush(5);
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_FULL);
}

template <class SPMCQueue>
void threadTest(int consumer, int producer, unsigned testSize) {
  static constexpr int maxMs = 5;
  std::random_device rd;
  std::uniform_int_distribution<> uid(0, maxMs);
  SPMCQueue q;
  std::atomic<int>* data = new std::atomic<int>[testSize];
  for (int i = 0; i < testSize; ++i) {
    data[i] = 0;
  }
  std::vector<std::thread> conTs(consumer);
  std::vector<std::thread> proTs(producer);
  std::atomic<bool> testPop = true;
  for (auto& t : conTs) {
    t = std::thread([&] {
      auto cons = q.createConsumer();
      while (testPop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(uid(rd)));
        auto tmp = cons.tryPop();
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
        if (uid(rd) % 2 == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(uid(rd)));
        }
        while(q.tryPush(i) != cxxtp::ts_queue::TS_DONE) {
          std::this_thread::yield();
        }
      }
    });
  }
  for (auto& t : proTs) {
    t.join();
  }
  auto sleepSec = maxMs * testSize / 1000 / consumer;
  sleepSec = sleepSec > 2 ? sleepSec : 2;
  std::this_thread::sleep_for(std::chrono::seconds(sleepSec));
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
  auto cons = q.createConsumer();
  auto tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_EMPTY);
  q.tryPush(1);
  q.tryPush(2);
  std::vector<int> v;
  tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  std::sort(v.begin(), v.end());
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  v.clear();
  q.tryPush(3);
  q.tryPush(4);
  tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_DONE);
  v.push_back(tmp.value());
  std::sort(v.begin(), v.end());
  EXPECT_EQ(v[0], 3);
  EXPECT_EQ(v[1], 4);
  tmp = cons.tryPop();
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_EMPTY);
  for (int i = 0; i < 5; ++i) {
    q.tryPush(i);
  }
  tmp = q.tryPush(5);
  EXPECT_EQ(tmp.status, cxxtp::ts_queue::TS_FULL);
}

TEST(VersionQueue, thread_test) {
  using Q = cxxtp::ts_queue::VersionQueue<int, 5>;
  threadTest<Q>(8, 1, 3);
  for (int i = 0; i < 10; ++i) {
    std::cout << "test round :" << i << std::endl;
    threadTest<Q>(8, 1, 1000);
  }
}