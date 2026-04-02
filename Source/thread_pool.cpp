#include "../include/thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads, BlockingQueue<Task> &queue)
    : queue_(queue), stop_(false) {
  for (size_t i = 0; i < num_threads; i++) {
    workers_.emplace_back(&ThreadPool::worker_loop, this);
  }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::worker_loop() {
  while (true) {
    // wait for a task
    auto task_opt = queue_.wait_and_pop();

    // if queue is shutdown and empty -> exit the thread
    if (!task_opt.has_value()) {
      return;
    }

    Task task = std::move(task_opt.value());

    try {
      task(); // Execute task
    } catch (const std::exception &e) {
      std::cerr << "Task failed: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Task failed: Unknown exception" << std::endl;
    }
  }
}

void ThreadPool::shutdown() {
  // Signal stop
  stop_ = true;

  // Shutdown queue -> wakes all waiting threads
  queue_.shutdown();

  // Wait for all threads to finish
  for (auto &worker : workers_) {
    worker.join();
  }
}

/*
Key Flow:

Worker thread:
    -> wait_and_pop()
    -> if shutdown -> exit
    -> else execute task

Shutdown:
    -> stop_ = true
    -> queue_.shutdown()
    -> join() all threads
*/