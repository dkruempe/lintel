#include "base_library/services/SchedulerService.h"
#include "base_library/services/LoggerService.h"

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

void SchedulerService::onInitialize() {
  LOG_INFO("start Scheduler with {} threads", numberOfThreads->getValue());
  threads.reserve(numberOfThreads->getValue());
  for (int i = 0; i < numberOfThreads->getValue(); i++) {
    threads.emplace_back([&] { run(); });
  }
}

SchedulerService::SchedulerService(
    const std::shared_ptr<PropertyService> &propertyService,
    const std::shared_ptr<ProcessName> &processName)
    : AbstractService<SchedulerService>(processName->getProcessName()),
      exit(false) {
  LOAD_PROPERTIES();
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