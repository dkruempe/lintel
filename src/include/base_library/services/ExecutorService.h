#ifndef LOGGING_EXECUTORSERVICE_H
#define LOGGING_EXECUTORSERVICE_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

class ExecutorService {
private:
  volatile bool exit = false;
  std::mutex mutex;
  std::condition_variable conditionVariable;
  std::thread runnable;
  typedef std::function<void()> TASK;
  std::queue<TASK> tasks;

  void run();

public:
  ExecutorService() : runnable([&]() { run(); }) {}

  ~ExecutorService() {
    exit = true;
    runnable.join();
  }

  template <class F, class... Args>
  std::future<typename std::result_of<
      typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
  execute(F &&f, Args &&... args);
};

template <class F, class... Args>
std::future<typename std::result_of<
    typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
ExecutorService::execute(F &&f, Args &&... args) {
  auto func = std::make_shared<
      std::packaged_task<typename std::result_of<typename std::decay<F>::type(
          typename std::decay<Args>::type...)>::type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  auto future = func->get_future();
  {
    std::unique_lock<std::mutex> lock(mutex);
    tasks.push([=, func = std::move(func)]() { (*func)(); });
  }
  conditionVariable.notify_one();
  return future;
}

#endif // LOGGING_EXECUTORSERVICE_H
