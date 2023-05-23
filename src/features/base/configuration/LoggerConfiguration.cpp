#include "base_library/features/base/configuration/LoggerConfiguration.h"

#include <utility>

LoggerConfiguration::LoggerConfiguration(
        std::string processName, std::string pattern, std::string level,
        bool isAsync, std::vector<LoggerSinkConfiguration> loggerSinks)
        : m_processName(std::move(processName)),
          m_pattern(std::move(pattern)),
          m_level(std::move(level)),
          m_isAsync(isAsync),
          m_loggerSinks(std::move(loggerSinks)) {}

const std::string &LoggerConfiguration::getProcessName() const {
    return m_processName;
}

const std::string &LoggerConfiguration::getPattern() const { return m_pattern; }

const std::string &LoggerConfiguration::getLevel() const { return m_level; }

bool LoggerConfiguration::isAsync() const { return m_isAsync; }

const std::vector<LoggerSinkConfiguration> &
LoggerConfiguration::getLoggerSinks() const {
    return m_loggerSinks;
}
