#include "lintel/features/base/configuration/HistoryServiceEntry.h"

#include <utility>

HistoryServiceEntry::HistoryServiceEntry(const std::string_view &component,
                           std::string processName, std::string queueName,
                           int32_t maxMessages)
        : Entry(component),
          m_processName(std::move(processName)),
          m_queueName(std::move(queueName)),
          m_maxMessages(maxMessages) {}

const std::string &HistoryServiceEntry::get_process_name() const {
    return m_processName;
}

const std::string &HistoryServiceEntry::get_queue_name() const {
    return m_queueName;
}

std::ostream &operator<<(std::ostream &os, const HistoryServiceEntry &entry) {
    os << static_cast<const Entry &>(entry)
       << " processName: " << entry.m_processName
       << " queueName: " << entry.m_queueName
       << " maxMessages: " << entry.m_maxMessages;
    return os;
}
