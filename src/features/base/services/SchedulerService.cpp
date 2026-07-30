#include "base_library/features/base/services/SchedulerService.h"

#include "base_library/core/services/LoggerService.h"

void SchedulerService::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.clear();
}

void SchedulerService::run() {
    while (!m_exit) {
        std::function<void()> funcTask;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto time = m_tasks.empty() ? std::chrono::steady_clock::now() +
                                          std::chrono::seconds(60)
                                        : m_tasks.front().time;

            m_conditionVariable.wait_until(lock, time, [&] {
                return m_exit || (!m_tasks.empty() && m_tasks.front().time != time);
            });
            if (m_exit && m_tasks.empty()) {
                return;
            }

            if (m_tasks.empty()) {
                continue;
            }

            if ((std::chrono::steady_clock::now() - m_tasks.front().time).count() <
                0) {
                continue;
            }

            std::pop_heap(m_tasks.begin(), m_tasks.end(), TaskComperator());
            Task task = std::move(m_tasks.back());
            m_tasks.pop_back();
            funcTask = task.func;
            if (task.period.count() != 0 && !m_exit) {
                task.time += task.period;
                m_tasks.push_back(task);
                std::push_heap(m_tasks.begin(), m_tasks.end(), TaskComperator());
            }
        }
        funcTask();
    }
}

void SchedulerService::onInitialize() {
    LOG_INFO("start Scheduler with {} threads", numberOfThreads->getValue());
    m_threads.reserve(static_cast<std::size_t>(numberOfThreads->getValue()));
    std::function<void()> func = [&]() { run(); };
    for (int i = 0; i < numberOfThreads->getValue(); i++) {
        std::thread temp(func);
        m_threads.push_back(std::move(temp));
    }
}

SchedulerService::SchedulerService(
        const std::shared_ptr<ProcessName> &processName)
        : PropertyRegistration(processName->getProcessName()),
          m_exit(false) {
    numberOfThreads = registerProperty<int>(
            "numberOfThreads", 40, "Number of Scheduler Threads", false,
            __FILE__, __LINE__);
}

SchedulerService::~SchedulerService() {
    {
        m_exit = true;
    }

    m_conditionVariable.notify_all();
    std::for_each(m_threads.begin(), m_threads.end(),
                  [&](std::thread &thread) { thread.join(); });
}