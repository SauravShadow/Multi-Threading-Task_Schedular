#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

// BlockingQueue<T> is a thread safe producer-consumer queue
// Producer -> push()
// Consumer -> wait_and_pop()
// Coordination -> mutex + condition_variable
// Shutdown-safe -> shutdown()

template <typename T>

class BlockingQueue {
private:
  std::queue<T> queue_;        // Store tasks (FIFO) (Shared Resource)
  std::mutex mutex_;           // Protects queue_
  std::condition_variable cv_; // Wakes up consumers
  bool shutdown_ = false;      // Signals all the thread to stop

public:
  BlockingQueue() = default;

  // Disable copy -> Mutex and condition variable should not be copied
  BlockingQueue(const BlockingQueue &) = delete;
  BlockingQueue &operator=(const BlockingQueue &) = delete;

  // Push new task
  void push(T value) {
    std::lock_guard<std::mutex> lock(
        mutex_); // Auto unclocks once goes out of scope
    queue_.push(std::move(value));
    cv_.notify_one(); // Wakes up one consumer thread
  }

  // Wait and pop (blocking)
  std::optional<T> wait_and_pop() {
    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });

    if (shutdown_ && queue_.empty())
      return std::nullopt;

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

  // Non-blocking pop
  bool try_pop(T &value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.empty() || shutdown_)
      return false;

    value = std::move(queue_.front());
    queue_.pop();
    return true;
  }

  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    cv_.notify_all();
  }

  bool empty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }
};

/*
Concurrency flow

Producer Thread:
    push(task) -> notify_one()

Consumer Thread
    wait() -> wake up() -> pop task
*/