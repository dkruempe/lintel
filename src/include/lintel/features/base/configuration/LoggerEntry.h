#ifndef LINTEL_LOGGERENTRY_H
#define LINTEL_LOGGERENTRY_H

#include <memory>
#include <ostream>

#include "lintel/features/base/configuration/Entry.h"
#include "lintel/features/base/configuration/LoggerConfiguration.h"
#include "lintel/features/base/configuration/LoggerPathConfiguration.h"

/** Configuration entry for a logger or logger path */
class LoggerEntry : public Entry
{
private:
  /** The logger configuration (if this is a logger entry) */
  std::shared_ptr<LoggerConfiguration> m_loggerConfiguration;
  /** The logger path configuration (if this is a path entry) */
  std::shared_ptr<LoggerPathConfiguration> m_loggerPathConfiguration;

public:
  /** Construct a logger configuration entry
   * @param component The configuration component name
   * @param loggerConfiguration The logger configuration */
  LoggerEntry(std::string_view component, std::shared_ptr<LoggerConfiguration> loggerConfiguration);

  /** Construct a logger path configuration entry
   * @param component The configuration component name
   * @param loggerPathConfiguration The logger path configuration */
  LoggerEntry(std::string_view component, std::shared_ptr<LoggerPathConfiguration> loggerPathConfiguration);

  /** Check if this entry contains a logger configuration
   * @return True if this is a logger configuration */
  [[nodiscard]] bool isLoggerConfiguration() const;

  /** Check if this entry contains a logger path configuration
   * @return True if this is a logger path configuration */
  [[nodiscard]] bool isLoggerPathConfiguration() const;

  /** Get the logger configuration
   * @return The logger configuration */
  [[nodiscard]] const std::shared_ptr<LoggerConfiguration> &getLoggerConfiguration() const;

  /** Get the logger path configuration
   * @return The logger path configuration */
  [[nodiscard]] const std::shared_ptr<LoggerPathConfiguration> &getLoggerPathConfiguration() const;
};

#endif// LINTEL_LOGGERENTRY_H
