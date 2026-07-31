#ifndef LOGGING_SCHEDULERSERVICE_H
#define LOGGING_SCHEDULERSERVICE_H

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <type_traits>     // wichtig für decay_t und invoke_result_t
#include <vector>

#include "base_library/core/services/PropertyRegistration.h"

/**
 * Service for scheduling and executing tasks at specified times or at fixed rates.
 */
class SchedulerService : public PropertyRegistration<SchedulerService>
{
private:
    /** A scheduled task with optional periodic re-execution. */
    struct Task
    {
        /**
         * Constructor for one-shot tasks.
         * @param time execution time
         * @param func task function
         */
        Task(std::chrono::steady_clock::time_point time, std::function<void()> func)
            : time(time), func(std::move(func)), period(std::chrono::seconds(0)) {}

        /**
         * Constructor for periodic tasks.
         * @param time initial execution time
         * @param func task function
         * @param period re-execution interval
         */
        Task(std::chrono::steady_clock::time_point time,
             std::function<void()> func,
             std::chrono::steady_clock::duration period)
            : time(time), func(std::move(func)), period(period) {}

        std::chrono::steady_clock::time_point time;   /**< Scheduled execution time */
        std::function<void()> func;                   /**< Task function */
        std::chrono::steady_clock::duration period;   /**< Re-execution period (zero for one-shot) */
    };

    struct TaskComperator
    {
        bool operator()(const Task& left, const Task& right) const
        {
            return right.time < left.time;
        }
    };

    std::vector<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    std::atomic<bool> m_exit{false};
    std::vector<std::thread> m_threads;

    void run();

    std::shared_ptr<Property<int>> numberOfThreads;

public:
    /**
     * Schedule a task for immediate execution.
     * @tparam F callable type
     * @tparam Args argument types
     * @param f callable to execute
     * @param args arguments to pass
     * @return future for the result
     */
    template<class F, class... Args>
    auto schedule(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    /**
     * Schedule a task for execution after a delay.
     * @tparam F callable type
     * @tparam Args argument types
     * @param d delay duration
     * @param f callable to execute
     * @param args arguments to pass
     * @return future for the result
     */
    template<class F, class... Args>
    auto schedule_after(const std::chrono::steady_clock::duration& d, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    /**
     * Schedule a task for execution at a specific time point.
     * @tparam F callable type
     * @tparam Args argument types
     * @param t absolute time point for execution
     * @param f callable to execute
     * @param args arguments to pass
     * @return future for the result
     */
    template<class F, class... Args>
    auto schedule_at(const std::chrono::steady_clock::time_point& t, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>;

    /**
     * Schedule a task for repeated execution at a fixed rate.
     * @tparam F callable type
     * @tparam Args argument types
     * @param d initial delay
     * @param period interval between executions
     * @param f callable to execute
     * @param args arguments to pass
     */
    template<class F, class... Args>
    void schedule_at_fixed_rate(const std::chrono::steady_clock::duration& d,
                                const std::chrono::steady_clock::duration& period,
                                F&& f, Args&&... args);

    /** Remove all scheduled tasks. */
    void clear();

    /**
     * Constructor.
     * @param processName current process name
     */
    explicit SchedulerService(const std::shared_ptr<ProcessName>& processName);
    void onInitialize() override;
    void onShutdown() override;
    virtual ~SchedulerService();
};

// ==================== Implementierungen ====================

template<class F, class... Args>
auto SchedulerService::schedule(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    return schedule_at(std::chrono::steady_clock::now(),
                       std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
auto SchedulerService::schedule_after(const std::chrono::steady_clock::duration& d,
                                      F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    return schedule_at(std::chrono::steady_clock::now() + d,
                       std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
auto SchedulerService::schedule_at(const std::chrono::steady_clock::time_point& t,
                                   F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
{
    using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    auto future = task->get_future();

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks.emplace_back(Task(t, [task = std::move(task)]() { (*task)(); }));
        std::push_heap(m_tasks.begin(), m_tasks.end(), TaskComperator());
    }

    m_conditionVariable.notify_one();
    return future;
}

template<class F, class... Args>
void SchedulerService::schedule_at_fixed_rate(const std::chrono::steady_clock::duration& d,
                                              const std::chrono::steady_clock::duration& period,
                                              F&& f, Args&&... args)
{
    auto bound_task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks.emplace_back(Task(std::chrono::steady_clock::now() + d,
                                  std::move(bound_task), period));
        std::push_heap(m_tasks.begin(), m_tasks.end(), TaskComperator());
    }

    m_conditionVariable.notify_one();
}

#endif // LOGGING_SCHEDULERSERVICE_H