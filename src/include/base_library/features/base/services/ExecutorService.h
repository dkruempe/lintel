#ifndef LOGGING_EXECUTORSERVICE_H
#define LOGGING_EXECUTORSERVICE_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

class ExecutorService {
 private:
  volatile bool m_exit = false;
  std::mutex m_mutex;
  std::condition_variable m_conditionVariable;
  std::thread m_runnable;
  typedef std::function<void()> TASK;
  std::queue<TASK> m_tasks;

  void run();

 public:
  ExecutorService() : m_runnable([&]() { run(); }) {}

  ~ExecutorService() {
    m_exit = true;
    m_runnable.join();
  }

  template <class F, class... Args>
  std::future<typename std::result_of<
      typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
  execute(F &&f, Args &&...args);
};

template <class F, class... Args>
std::future<typename std::result_of<
    typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
ExecutorService::execute(F &&f, Args &&...args) {
  auto func = std::make_shared<
      std::packaged_task<typename std::result_of<typename std::decay<F>::type(
          typename std::decay<Args>::type...)>::type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  auto future = func->get_future();
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.push([=, func = std::move(func)]() { (*func)(); });
  }
  m_conditionVariable.notify_one();
  return future;
}

#endif  // LOGGING_EXECUTORSERVICE_H
