#ifndef CPP_BASE_LIBRARY_LOGGERCONFIGURATION_H
#define CPP_BASE_LIBRARY_LOGGERCONFIGURATION_H

#include <string>
#include <vector>

#include "base_library/features/base/configuration/LoggerSinkConfiguration.h"

class LoggerConfiguration {
 private:
  std::string m_processName;
  std::string m_pattern;
  std::string m_level;
  bool m_isAsync;
  std::vector<LoggerSinkConfiguration> m_loggerSinks;

 public:
  LoggerConfiguration(std::string processName, std::string pattern,
                      std::string level, bool isAsync,
                      std::vector<LoggerSinkConfiguration> loggerSinks);

  [[nodiscard]] const std::string& getProcessName() const;
  [[nodiscard]] const std::string& getPattern() const;
  [[nodiscard]] const std::string& getLevel() const;
  [[nodiscard]] bool isAsync() const;
  [[nodiscard]] const std::vector<LoggerSinkConfiguration>& getLoggerSinks()
      const;
};

#endif  // CPP_BASE_LIBRARY_LOGGERCONFIGURATION_H
