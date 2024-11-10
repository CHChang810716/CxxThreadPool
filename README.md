# C++ coroutine based thread pool 

## Prerequisite

* CMake >= 3.21.0
* C++20 compiler

## Example - Paralle Sort

```c++
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

int main() {
  std::random_device rd;
  std::uniform_int_distribution<unsigned> dist(0);
  unsigned dataSize = 1000 * 1000 * 10;
  Data data(dataSize);
  for (auto& d : data) {
    d = dist(rd);
  }
  cxxtp::Scheduler sched(4);
  auto fut = sched.async(parallelSort, data.begin(), data.end());
  // Since main thread is not a coroutine, we provide Scheduler::await for main thread.
  sched.await(fut);
  
}
```