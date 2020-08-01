#include "base_library/services/SchedulerService.h"

void SchedulerService::clear() {
  std::lock_guard<std::mutex> lock(mutex);
  tasks.clear();
}

void SchedulerService::run() {

  while (!exit || !tasks.empty()) {
    auto time = tasks.empty() ? std::chrono::steady_clock::now() +
                                    std::chrono::seconds(60)
                              : tasks.front().time;
    std::function<void()> funcTask;
    {
      std::unique_lock<std::mutex> lock(mutex);

      conditionVariable.wait_until(lock, time, [&] {
        return exit || (!tasks.empty() && tasks.front().time != time);
      });
      if (exit && tasks.empty()) {
        return;
      }

      if (tasks.empty()) {
        continue;
      }

      if ((std::chrono::steady_clock::now() - tasks.front().time).count() < 0) {
        continue;
      }

      std::pop_heap(tasks.begin(), tasks.end(), TaskComperator());
      Task task = std::move(tasks.back());
      tasks.pop_back();
      funcTask = task.func;
      if (task.period.count() != 0 && !exit) {
        task.time += task.period;
        tasks.push_back(task);
        std::push_heap(tasks.begin(), tasks.end(), TaskComperator());
      }
    }
    funcTask();
  }
}

SchedulerService::SchedulerService(int numberOfThreads)
    : exit(false), numberOfThreads(numberOfThreads) {
  threads.reserve(numberOfThreads);
  for (int i = 0; i < numberOfThreads; i++) {
    threads.emplace_back([&] { run(); });
  }
}

SchedulerService::~SchedulerService() {
  {
    std::unique_lock<std::mutex> lock(mutex);
    exit = true;
  }

  conditionVariable.notify_all();
  std::for_each(threads.begin(), threads.end(),
                [&](std::thread &thread) { thread.join(); });
}