#include "base_library/features/base/models/HistoryEntry.h"

HistoryEntry::HistoryEntry(
    std::string processName, std::string serviceName, std::string label,
    std::string text,
    const date::sys_time<std::chrono::microseconds> &createdTimestamp)
    : m_processName(std::move(processName)),
      m_serviceName(std::move(serviceName)),
      m_label(std::move(label)),
      m_text(std::move(text)),
      m_createdTimestamp(createdTimestamp) {}

HistoryEntry::HistoryEntry(const AbstractServiceInterface &service,
                           std::string label, std::string text)
    : m_processName(service.getProcessName()),
      m_serviceName(std::string(service.getClassName())),
      m_label(std::move(label)),
      m_text(std::move(text)),
      m_createdTimestamp(std::chrono::system_clock::now()) {}

const date::sys_time<std::chrono::microseconds>
    &HistoryEntry::getCreatedTimestamp() const {
  return m_createdTimestamp;
}

const std::string &HistoryEntry::getLabel() const { return m_label; }

const std::string &HistoryEntry::getProcessName() const {
  return m_processName;
}

const std::string &HistoryEntry::getServiceName() const {
  return m_serviceName;
}

const std::string &HistoryEntry::getText() const { return m_text; }

bool HistoryEntry::operator==(const HistoryEntry &rhs) const {
  return m_processName == rhs.m_processName &&
         m_serviceName == rhs.m_serviceName && m_label == rhs.m_label &&
         m_text == rhs.m_text && m_createdTimestamp == rhs.m_createdTimestamp;
}
bool HistoryEntry::operator!=(const HistoryEntry &rhs) const {
  return !(rhs == *this);
}
