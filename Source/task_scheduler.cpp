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

    auto &next_task = delay_queue.top();
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

template <typename F>
auto TaskScheduler::submit(F &&f) -> std::future<decltype(f())> {
  using ReturnType = decltype(f());

  auto task_ptr =
      std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));

  std::future<ReturnType> future = task_ptr->get_future();

  ready_queue.push([task_ptr]() { (*task_ptr)(); });

  return future;
}

template <typename F>
auto TaskScheduler::schedule_after(F &&f, std::chrono::milliseconds delay)
    -> std::future<decltype(f())> {
  using ReturnType = decltype(f());

  auto task_ptr =
      std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));

  std::future<ReturnType> future = task_ptr->get_future();

  ScheduledTask task;
  task.execute_at = std::chrono::steady_clock::now() + delay;
  task.func = [task_ptr]() { (*task_ptr)(); };

  {
    std::lock_guard<std::mutex> lock(delay_mtx_);
    delay_queue.push(std::move(task));
  }

  return future;
}