#include <gtest/gtest.h>

#include <coroutine>

#include "cxxtp/Future.hpp"
#include "cxxtp/TSQueue.hpp"
#include "cxxtp/Task.hpp"

struct DummyScheduleAgent {
  void deleteFuncCtx() {}
  void sched(cxxtp::Task&& task) {
    auto obj = queue->tryPush(std::move(task));
    assert(!obj.has_value());
  }
  void suspend(cxxtp::Task&& task) {
    auto obj = queue->tryPush(std::move(task));
    assert(!obj.has_value());
  }
  void runTask() {
    auto t = queue->tryPop();
    assert(t.has_value());
    t.value()();
  }
  cxxtp::TSQueue<cxxtp::Task, 4>* queue;
};

TEST(Future, coroutine) {
  cxxtp::TSQueue<cxxtp::Task, 4> queue;
  DummyScheduleAgent agent{&queue};
  int x = 10;
  auto coro = [&x](DummyScheduleAgent sched)
      -> cxxtp::Future<int, DummyScheduleAgent> { co_return x + 1; };
  auto fut = coro(agent);
  if (!fut.await_ready()) {
    fut.getCoroutineHandle().resume();
  }
  EXPECT_EQ(fut.await_resume(), 11);
}

cxxtp::Future<int, DummyScheduleAgent> coro1(DummyScheduleAgent& sched,
                                             int x) {
  co_return x + 1;
}
TEST(Future, coroutine_await) {
  cxxtp::TSQueue<cxxtp::Task, 4> queue;
  DummyScheduleAgent agent{&queue};
  {
    // x = 10;
    auto coro2 = [](DummyScheduleAgent sched)
        -> cxxtp::Future<void, DummyScheduleAgent> {
      auto m = co_await coro1(sched, 10);
      EXPECT_TRUE(false && "should not be here");
      co_return;
    };
    auto fut2 = coro2(agent);
    agent.sched([coro = fut2.getCoroutineHandle()]() { coro.resume(); });
  }
  agent.runTask();
  EXPECT_GT(agent.queue->size(), 0);
  agent.runTask();
  EXPECT_GT(agent.queue->size(), 0);
  agent.runTask();
  EXPECT_GT(agent.queue->size(), 0);
}

TEST(Future, coroutine_await2) {
  cxxtp::TSQueue<cxxtp::Task, 4> queue;
  DummyScheduleAgent agent{&queue};
  bool flag = false;
  {
    auto coro2 = [&flag](DummyScheduleAgent sched)
        -> cxxtp::Future<void, DummyScheduleAgent> {
      auto fut1 = coro1(sched, 10);
      sched.sched([coro = fut1.getCoroutineHandle()]() { coro.resume(); });
      int n = co_await fut1;
      EXPECT_EQ(n, 11);
      flag = true;
      co_return;
    };
    auto fut2 = coro2(agent);
    agent.sched([coro = fut2.getCoroutineHandle()]() { coro.resume(); });
  }
  while (agent.queue->size() > 0) {
    agent.runTask();
  }
  EXPECT_TRUE(flag);
}