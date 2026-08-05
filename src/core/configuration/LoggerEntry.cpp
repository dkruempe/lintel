#include "base_library/core/configuration/LoggerEnrty.h"

LoggerEntry::LoggerEntry(
        std::string_view component,
        std::shared_ptr<LoggerConfiguration> loggerConfiguration)
        : Entry(component),
          m_loggerConfiguration(std::move(loggerConfiguration)),
          m_loggerPathConfiguration(nullptr) {}

LoggerEntry::LoggerEntry(
        std::string_view component,
        std::shared_ptr<LoggerPathConfiguration> loggerPathConfiguration)
        : Entry(component),
          m_loggerConfiguration(nullptr),
          m_loggerPathConfiguration(std::move(loggerPathConfiguration)) {}

bool LoggerEntry::isLoggerConfiguration() const {
    return m_loggerConfiguration != nullptr;
}

bool LoggerEntry::isLoggerPathConfiguration() const {
    return m_loggerPathConfiguration != nullptr;
}

const std::shared_ptr<LoggerConfiguration> &
LoggerEntry::getLoggerConfiguration() const {
    return m_loggerConfiguration;
}

const std::shared_ptr<LoggerPathConfiguration> &
LoggerEntry::getLoggerPathConfiguration() const {
    return m_loggerPathConfiguration;
}
