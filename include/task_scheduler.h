#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "blocking_queue.h"
#include "thread_pool.h"

// Task Wrapper
struct ScheduledTask {
  std::chrono::steady_clock::time_point execute_at;
  std::function<void> func;

  bool operator>(const ScheduledTask &other) const {
    return execute_at > other.execute_at;
  }
};

class TaskScheduler {
private:
  BlockingQueue<std::function<void()>> ready_queue;
  std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, std::greater<>>
      delay_queue;

  std::mutex delay_mtx_;
  std::thread timer_thread_;
  std::atomic<bool> stop_;

  ThreadPool thread_pool_;

  void timer_loop();

public:
  TaskScheduler(size_t num_threads);
  ~TaskScheduler();

  template <typename F> auto submit(F &&f) -> std::future<decltype(f())>;

  template <typename F>
  auto schedule_after(F &&f, std::chrono::milliseconds delay)
      -> std::future<decltype(f())>;

  void shutdown();
};
