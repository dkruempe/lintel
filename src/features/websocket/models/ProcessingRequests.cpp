#include "base_library/features/websocket/models/ProcessingRequests.h"
void ProcessingRequests::waitingFor(const std::string& method,
                                    const std::string& id) {
  std::lock_guard<std::mutex> locker(m_processingRequestsMutex);
  m_processingRequests.insert({id, method});
}
std::optional<std::string> ProcessingRequests::acknowledgeOf(
    const std::string& id) {
  std::lock_guard<std::mutex> locker(m_processingRequestsMutex);
  auto found = m_processingRequests.find(id);
  if (found == m_processingRequests.end()) {
    return std::nullopt;
  }
  auto optional = std::make_optional(found->second);
  m_processingRequests.erase(found->first);
  return optional;
}
