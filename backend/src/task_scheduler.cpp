#include "../include/task_scheduler.h"
#include <iostream>

TaskScheduler::TaskScheduler(size_t num_threads)
    : thread_pool_(num_threads, ready_queue), stop_(false) {
  timer_thread_ = std::thread(&TaskScheduler::timer_loop, this);
}

TaskScheduler::~TaskScheduler() { shutdown(); }

void TaskScheduler::timer_loop() {
  std::unique_lock<std::mutex> lock(delay_mtx_);

  while (!stop_) {
    if (delay_queue.empty()) {
      // Sleep until a task is scheduled or shutdown is requested (no busy-poll).
      timer_cv_.wait(lock, [this] { return stop_ || !delay_queue.empty(); });
      continue;
    }

    auto next_time = delay_queue.top().execute_at;
    auto now = std::chrono::steady_clock::now();

    if (next_time <= now) {
      // Time to execute: copy the task out, pop, then push to the ready queue
      // without holding delay_mtx_.
      auto func = delay_queue.top().func;
      delay_queue.pop();
      lock.unlock();
      ready_queue.push(func);
      lock.lock();
    } else {
      // Wait until the next task is due. A new earlier task or shutdown wakes us
      // early via timer_cv_, so shutdown latency is not bound to this deadline.
      timer_cv_.wait_until(lock, next_time);
    }
  }
}

void TaskScheduler::shutdown() {
  {
    std::lock_guard<std::mutex> lock(delay_mtx_);
    stop_ = true;
  }
  timer_cv_.notify_all(); // Wake the timer immediately, regardless of any pending sleep

  if (timer_thread_.joinable())
    timer_thread_.join();

  ready_queue.shutdown();
  thread_pool_.shutdown();
}
