#pragma once
#include "../include/blocking_queue.h"
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

using Task = std::function<void()>;

class ThreadPool {
private:
  std::vector<std::thread> workers_;
  BlockingQueue<Task> &queue_;
  std::atomic<bool> stop_;

  void worker_loop();

public:
  ThreadPool(size_t num_threads,
             BlockingQueue<Task> &queue); // ThreadPool does not own queue
  ~ThreadPool();

  void shutdown();
};
