#ifndef LOGGING_EXECUTORSERVICE_H
#define LOGGING_EXECUTORSERVICE_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

/**
 * A simple thread pool executor that queues and executes tasks asynchronously.
 */
class ExecutorService {
private:
    volatile bool m_exit = false;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    std::thread m_runnable;
    typedef std::function<void()> TASK;
    std::queue<TASK> m_tasks;

    /** Main loop that processes queued tasks. */
    void run();

public:
    /** Constructor; starts the worker thread. */
    ExecutorService() : m_runnable([&]() { run(); }) {}

    /** Destructor; signals exit and joins the worker thread. */
    ~ExecutorService() {
        m_exit = true;
        m_runnable.join();
    }

    /**
     * Enqueue a callable for asynchronous execution.
     * @tparam F callable type
     * @tparam Args argument types
     * @param f callable to execute
     * @param args arguments to pass to the callable
     * @return a future that will hold the result
     */
    template<class F, class... Args>
    std::future<typename std::result_of<
            typename std::decay<F>::type(typename std::decay<Args>::type...)>::type>
    execute(F &&f, Args &&...args);
};

template<class F, class... Args>
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
