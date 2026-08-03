#include "base_library/features/base/services/ExecutorService.h"

#include "base_library/core/services/LoggerService.h"

void ExecutorService::run() {
    while (true) {
        TASK task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_conditionVariable.wait(lock,
                                     [&] { return m_exit || !m_tasks.empty(); });
            if (m_exit && m_tasks.empty()) {
                return;
            }
            if (m_tasks.empty()) {
                continue;
            }
            task = m_tasks.front();
            m_tasks.pop();
        }
        try {
            task();
        } catch (const std::exception &exception) {
            LOG_ERROR("executor task failed: {}", exception.what());
        } catch (...) {
            LOG_ERROR("executor task failed with unknown exception");
        }
    }
}
