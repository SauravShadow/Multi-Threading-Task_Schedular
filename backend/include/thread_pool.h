#pragma once
#include "../include/blocking_queue.h"
#include <functional>
#include <thread>
#include <vector>

using Task = std::function<void()>;

class ThreadPool {
private:
  std::vector<std::thread> workers_;
  BlockingQueue<Task> &queue_;

  void worker_loop();

public:
  ThreadPool(size_t num_threads,
             BlockingQueue<Task> &queue); // ThreadPool does not own queue
  ~ThreadPool();

  void shutdown();
};
