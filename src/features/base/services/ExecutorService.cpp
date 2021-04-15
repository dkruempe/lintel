#include "base_library/features/base/services/ExecutorService.h"
void ExecutorService::run() {
  while (!exit || !tasks.empty()) {
    TASK task;
    {
      std::unique_lock<std::mutex> lock(mutex);
      auto time = std::chrono::steady_clock::now() + std::chrono::seconds(60);
      conditionVariable.wait_until(lock, time,
                                   [&] { return exit || !tasks.empty(); });
      if (exit && tasks.empty()) {
        return;
      }
      if (tasks.empty()) {
        continue;
      }
      task = tasks.front();
      tasks.pop();
    }
    task();
  }
}
