#include "base_library/features/base/services/ExecutorService.h"

void ExecutorService::run() {
    while (!m_exit || !m_tasks.empty()) {
        TASK task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto time = std::chrono::steady_clock::now() + std::chrono::seconds(60);
            m_conditionVariable.wait_until(
                    lock, time, [&] { return m_exit || !m_tasks.empty(); });
            if (m_exit && m_tasks.empty()) {
                return;
            }
            if (m_tasks.empty()) {
                continue;
            }
            task = m_tasks.front();
            m_tasks.pop();
        }
        task();
    }
}
