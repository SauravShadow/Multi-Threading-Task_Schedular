#include "../include/task_scheduler.h"
#include <iostream>

int main() {
  TaskScheduler scheduler(4);

  // Submit normal tasks
  for (int i = 0; i < 10; i++) {
    scheduler.submit([i]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::cout << "Task " << i << " executed by thread "
                << std::this_thread::get_id() << std::endl;
    });
  }

  // Submit delayed tasks
  for (int i = 10; i < 20; i++) {
    scheduler.schedule_after(
        [i]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          std::cout << "Delayed Task " << i << " executed by thread "
                    << std::this_thread::get_id() << std::endl;
        },
        std::chrono::milliseconds((i - 10) * 100));
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));
  scheduler.shutdown();

  return 0;
}