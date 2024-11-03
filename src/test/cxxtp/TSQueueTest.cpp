#include <gtest/gtest.h>

#include <algorithm>
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

template <class SCMPQueue>
void scmpTest(SCMPQueue& q) {
  std::vector<std::atomic<int>> data;
  std::vector<std::thread> ts(8);
  for (int i = 0; i < 8; ++i) {
    ts[i] = std::thread([&] {
      while (true) {
        auto tmp = q.tryPop();
        if (tmp.status == cxxtp::ts_queue::TS_DONE) {
          data[tmp.value()]++;
        }
      }
    });
  }
  for (auto& t : ts) {
    t.join();
  }
}
TEST(VersionQueue, basic_test) {
  cxxtp::ts_queue::VersionQueue<int, 5> q;
  basicTest(q);
}