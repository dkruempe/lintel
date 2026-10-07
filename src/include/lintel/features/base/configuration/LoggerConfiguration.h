#ifndef LINTEL_LOGGERCONFIGURATION_H
#define LINTEL_LOGGERCONFIGURATION_H

#include <string>
#include <vector>

#include "lintel/features/base/configuration/LoggerSinkConfiguration.h"

/** Configuration for a single logger instance */
class LoggerConfiguration {
private:
    /** The process name associated with this logger */
    std::string m_processName;
    /** The log pattern */
    std::string m_pattern;
    /** The log level */
    std::string m_level;
    /** Whether the logger is asynchronous */
    bool m_isAsync;
    /** The logger sink configurations */
    std::vector<LoggerSinkConfiguration> m_loggerSinks;

public:
    /** Construct a logger configuration
     * @param processName The process name
     * @param pattern The log pattern
     * @param level The log level
     * @param isAsync Whether logging is asynchronous
     * @param loggerSinks The sink configurations */
    LoggerConfiguration(std::string processName, std::string pattern,
                        std::string level, bool isAsync,
                        std::vector<LoggerSinkConfiguration> loggerSinks);

    /** Get the process name
     * @return The process name */
    [[nodiscard]] const std::string &getProcessName() const;

    /** Get the log pattern
     * @return The pattern string */
    [[nodiscard]] const std::string &getPattern() const;

    /** Get the log level
     * @return The level string */
    [[nodiscard]] const std::string &getLevel() const;

    /** Check if logging is asynchronous
     * @return True if async */
    [[nodiscard]] bool isAsync() const;

    /** Get the logger sink configurations
     * @return Vector of sink configurations */
    [[nodiscard]] const std::vector<LoggerSinkConfiguration> &getLoggerSinks()
    const;
};

#endif  // LINTEL_LOGGERCONFIGURATION_H
