#ifndef LOGGING_SCHEDULERSERVICE_H
#define LOGGING_SCHEDULERSERVICE_H
#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/property/services/PropertyService.h"

class SchedulerService : public AbstractService<SchedulerService> {
private:
  struct Task {
    Task(std::chrono::steady_clock::time_point time, std::function<void()> func)
        : time(time), func(std::move(func)), period(std::chrono::seconds(0)) {}

    Task(std::chrono::steady_clock::time_point time, std::function<void()> func,
         std::chrono::steady_clock::duration period)
        : time(time), func(std::move(func)), period(period) {}

    std::chrono::steady_clock::time_point time; // next execute
    std::function<void()> func;                 // task to be executed
    // period 0 => no period > 0 period schedule until process end
    std::chrono::steady_clock::duration period;
  };

  struct TaskComperator {
    // comperator
    bool operator()(const Task &left, const Task &right) const {
      return right.time < left.time;
    }
  };

  std::vector<Task> tasks;
  std::mutex mutex;
  std::condition_variable conditionVariable;
  volatile bool exit;
  std::vector<std::thread> threads;

  void run();

  DEFINE_PROPERTY(numberOfThreads, int, 40, "Number of Scheduler Threads",
                  false);

public:
  template <class F, class... Args>
  std::future<typename std::result_of<
      typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
  schedule(F &&f, Args &&...args);

  template <class F, class... Args>
  std::future<typename std::result_of<
      typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
  schedule_after(const std::chrono::steady_clock::duration &d, F &&f,
                 Args &&...args);

  template <class F, class... Args>
  std::future<typename std::result_of<
      typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
  schedule_at(const std::chrono::steady_clock::time_point &t, F &&f,
              Args &&...args);

  template <class F, class... Args>
  void schedule_at_fixed_rate(const std::chrono::steady_clock::duration &d,
                              const std::chrono::steady_clock::duration &period,
                              F &&f, Args &&...args);

  void clear();

  explicit SchedulerService(const std::shared_ptr<ProcessName> &processName);

  void onInitialize() override;

  ~SchedulerService();
};

template <class F, class... Args>
std::future<typename std::result_of<
    typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
SchedulerService::schedule(F &&f, Args &&...args) {
  return schedule_at(std::chrono::steady_clock::now(), std::forward<F>(f),
                     std::forward<Args>(args)...);
}

template <class F, class... Args>
std::future<typename std::result_of<
    typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
SchedulerService::schedule_after(const std::chrono::steady_clock::duration &d,
                                 F &&f, Args &&...args) {
  return schedule_at(std::chrono::steady_clock::now() + d, std::forward<F>(f),
                     std::forward<Args>(args)...);
}

template <class F, class... Args>
void SchedulerService::schedule_at_fixed_rate(
    const std::chrono::steady_clock::duration &d,
    const std::chrono::steady_clock::duration &period, F &&f, Args &&...args) {
  {
    std::unique_lock<std::mutex> lock(mutex);
    tasks.emplace_back(Task(
        std::chrono::steady_clock::now() + d,
        std::bind(std::forward<F>(f), std::forward<Args>(args)...), period));
    std::push_heap(tasks.begin(), tasks.end(), TaskComperator());
  }
  conditionVariable.notify_one();
}

template <class F, class... Args>
std::future<typename std::result_of<
    typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
SchedulerService::schedule_at(const std::chrono::steady_clock::time_point &t,
                              F &&f, Args &&...args) {
  auto function = std::make_shared<
      std::packaged_task<typename std::result_of<typename std::decay<F>::type(
          typename std::decay<Args>::type...)>::type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  auto future = function->get_future();
  {
    std::unique_lock<std::mutex> lock(mutex);
    tasks.emplace_back(Task(t, [=, func = std::move(function)] { (*func)(); }));
    std::push_heap(tasks.begin(), tasks.end(), TaskComperator());
  }

  conditionVariable.notify_one();
  return future;
}

#endif // LOGGING_SCHEDULERSERVICE_H
