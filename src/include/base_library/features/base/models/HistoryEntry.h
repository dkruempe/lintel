#ifndef CPP_BASE_LIBRARY_HISTORYENTRY_H
#define CPP_BASE_LIBRARY_HISTORYENTRY_H

#include <date/date.h>
#include <date/tz.h>

#include <string>
#include <utility>

#include "base_library/core/services/AbstractService.h"

#define DEFINE_HISTORY_ENTRY(name, label, text) \
  HistoryEntry name = HistoryEntry(*this, label, text)

class HistoryEntry {
private:
    std::string m_processName;
    std::string m_serviceName;
    std::string m_label;
    std::string m_text;
    std::string m_uuid;
    date::sys_time<std::chrono::microseconds> m_createdTimestamp;

public:
    HistoryEntry(
            std::string processName, std::string serviceName, std::string label,
            std::string text,
            const date::sys_time<std::chrono::microseconds> &createdTimestamp);

    HistoryEntry(const AbstractServiceInterface &service, std::string label,
                 std::string text);

    [[nodiscard]] const std::string &getProcessName() const;

    [[nodiscard]] const std::string &getServiceName() const;

    [[nodiscard]] const std::string &getLabel() const;

    [[nodiscard]] const std::string &getText() const;

    [[nodiscard]] const std::string &getUuid() const;

    [[nodiscard]] const date::sys_time<std::chrono::microseconds> &
    getCreatedTimestamp() const;

    bool operator==(const HistoryEntry &rhs) const;

    bool operator!=(const HistoryEntry &rhs) const;
};

#endif  // CPP_BASE_LIBRARY_HISTORYENTRY_H
