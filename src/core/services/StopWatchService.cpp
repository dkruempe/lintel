#include "base_library/core/services/StopWatchService.h"

StopWatchService::StopWatchService(bool run) : m_run(run) {
  if (run) {
    reset();
  }
}

void StopWatchService::reset() {
  m_run = true;
  m_startTime = std::chrono::steady_clock::now();
  m_stopVar = false;
}

void StopWatchService::stop() {
  if (m_stopVar) {
    return;
  }
  m_endTime = std::chrono::steady_clock::now();
  m_stopVar = true;
  m_run = false;
}

std::chrono::nanoseconds StopWatchService::elapsed() const {
  if (m_stopVar) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(m_endTime -
                                                                m_startTime);
  }
  if (!m_run) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(m_startTime -
                                                                m_startTime);
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - m_startTime);
}
std::ostream &operator<<(std::ostream &os, const StopWatchService &service) {
  return os << static_cast<double>(service.elapsed().count()) / 1000000.0
            << " ms";
}
