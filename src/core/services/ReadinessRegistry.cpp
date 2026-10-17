#include "lintel/core/services/ReadinessRegistry.h"

#include <exception>
#include <utility>

namespace lintel {
ReadinessRegistry &ReadinessRegistry::instance()
{
  static ReadinessRegistry registry;
  return registry;
}

void ReadinessRegistry::registerCheck(std::string name, Check check)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &entry : m_checks) {
    if (entry.first == name) {
      entry.second = std::move(check);
      return;
    }
  }
  m_checks.emplace_back(std::move(name), std::move(check));
}

void ReadinessRegistry::clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_checks.clear();
}

std::vector<ReadinessRegistry::CheckResult> ReadinessRegistry::run() const
{
  // snapshot under the lock, execute outside of it: checks may block on their
  // dependency and must not serialize registration of other checks
  std::vector<std::pair<std::string, Check>> checks;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    checks = m_checks;
  }
  std::vector<CheckResult> results;
  results.reserve(checks.size());
  for (const auto &entry : checks) {
    CheckResult result;
    result.m_name = entry.first;
    try {
      std::string detail;
      if (entry.second == nullptr) {
        result.m_ready = false;
        result.m_detail = "check is empty";
      } else {
        result.m_ready = entry.second(detail);
        if (!result.m_ready) { result.m_detail = detail.empty() ? "check reported not ready" : detail; }
      }
    } catch (const std::exception &exception) {
      result.m_ready = false;
      result.m_detail = exception.what();
    } catch (...) {
      result.m_ready = false;
      result.m_detail = "unknown exception";
    }
    results.push_back(std::move(result));
  }
  return results;
}
}// namespace lintel
