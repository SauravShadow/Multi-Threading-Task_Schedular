#include "../include/task_scheduler.h"
#include <iostream>

TaskScheduler::TaskScheduler(size_t num_threads)
    : thread_pool_(num_threads, ready_queue), stop_(false) {
  timer_thread_ = std::thread(&TaskScheduler::timer_loop, this);
}

TaskScheduler::~TaskScheduler() { shutdown(); }

void TaskScheduler::timer_loop() {
  while (!stop_) {
    std::unique_lock<std::mutex> lock(delay_mtx_);

    if (delay_queue.empty()) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    auto next_task = delay_queue.top(); // Make a deliberate copy to avoid use-after-free
    auto now = std::chrono::steady_clock::now();

    if (next_task.execute_at <= now) {
      // Time to execute
      delay_queue.pop();
      lock.unlock();
      ready_queue.push(next_task.func);
    } else {
      auto sleep_time = next_task.execute_at - now;
      lock.unlock();
      std::this_thread::sleep_for(sleep_time);
    }
  }
}

void TaskScheduler::shutdown() {
  stop_ = true;

  if (timer_thread_.joinable())
    timer_thread_.join();

  ready_queue.shutdown();
  thread_pool_.shutdown();
}
