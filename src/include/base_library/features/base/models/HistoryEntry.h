#ifndef CPP_BASE_LIBRARY_HISTORYENTRY_H
#define CPP_BASE_LIBRARY_HISTORYENTRY_H

#include <date/date.h>
#include <date/tz.h>

#include <string>
#include <utility>

#include "base_library/core/services/AbstractService.h"

/**
 * Represents a single history entry with process/service context, a label, text, and timestamp.
 */
class HistoryEntry
{
private:
  std::string m_processName;
  std::string m_serviceName;
  std::string m_label;
  std::string m_text;
  std::string m_uuid;
  date::sys_time<std::chrono::microseconds> m_createdTimestamp;

public:
  /**
   * Constructor.
   * @param processName name of the process
   * @param serviceName name of the service
   * @param label entry label
   * @param text entry text
   * @param createdTimestamp creation timestamp
   */
  HistoryEntry(std::string processName,
    std::string serviceName,
    std::string label,
    std::string text,
    const date::sys_time<std::chrono::microseconds> &createdTimestamp);

  /**
   * Constructor inferring process and service name from an AbstractServiceInterface.
   * @param service the service interface
   * @param label entry label
   * @param text entry text
   */
  HistoryEntry(const AbstractServiceInterface &service, std::string label, std::string text);

  /** @return process name */
  [[nodiscard]] const std::string &getProcessName() const;

  /** @return service name */
  [[nodiscard]] const std::string &getServiceName() const;

  /** @return entry label */
  [[nodiscard]] const std::string &getLabel() const;

  /** @return entry text */
  [[nodiscard]] const std::string &getText() const;

  /** @return entry UUID */
  [[nodiscard]] const std::string &getUuid() const;

  /** @return creation timestamp */
  [[nodiscard]] const date::sys_time<std::chrono::microseconds> &getCreatedTimestamp() const;

  /** @return true if entries are equal */
  bool operator==(const HistoryEntry &rhs) const;

  /** @return true if entries are not equal */
  bool operator!=(const HistoryEntry &rhs) const;
};

#endif// CPP_BASE_LIBRARY_HISTORYENTRY_H
