#ifndef CPP_BASE_LIBRARY_LOGGERENRTY_H
#define CPP_BASE_LIBRARY_LOGGERENRTY_H

#include <memory>
#include <ostream>

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/base/configuration/LoggerConfiguration.h"
#include "base_library/features/base/configuration/LoggerPathConfiguration.h"

class LoggerEntry : public Entry {
 private:
  std::shared_ptr<LoggerConfiguration> m_loggerConfiguration;
  std::shared_ptr<LoggerPathConfiguration> m_loggerPathConfiguration;

 public:
  LoggerEntry(std::string_view component,
              std::shared_ptr<LoggerConfiguration> loggerConfiguration);

  LoggerEntry(std::string_view component,
              std::shared_ptr<LoggerPathConfiguration> loggerPathConfiguration);

  [[nodiscard]] bool isLoggerConfiguration() const;
  [[nodiscard]] bool isLoggerPathConfiguration() const;
  [[nodiscard]] const std::shared_ptr<LoggerConfiguration>&
  getLoggerConfiguration() const;
  [[nodiscard]] const std::shared_ptr<LoggerPathConfiguration>&
  getLoggerPathConfiguration() const;
};

#endif  // CPP_BASE_LIBRARY_LOGGERENRTY_H
