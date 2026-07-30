#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 *
 * Uses <spdlog/logger.h> instead of <spdlog/spdlog.h> to reduce
 * transitive includes (avoids registry, synchronous_factory).
 * With PCH enabled, all of spdlog is parsed only once.
 */
#include <spdlog/logger.h>

class Configuration;
#include "base_library/features/base/models/ProcessName.h"

/** Singleton service providing spdlog-based logging for the application. */
class LoggerService
{
private:
  // variables
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<spdlog::logger> m_logger;

  /** Initialize the spdlog logger instance. */
  std::shared_ptr<spdlog::logger> init();

  static std::unique_ptr<LoggerService> m_instance;
  static std::once_flag m_initInstanceFlag;

public:
  /** Default constructor. */
  LoggerService();

  /** Construct a LoggerService with the given process name and configuration.
   * @param processName  the process name
   * @param configuration the application configuration */
  LoggerService(std::shared_ptr<ProcessName> processName, std::shared_ptr<Configuration> configuration);

  /** Destructor. */
  ~LoggerService();

  /** Get or create the singleton logger service instance.
   * @param processInfo   the process name information
   * @param configuration the application configuration
   * @return reference to the singleton LoggerService */
  static LoggerService &getOrCreate(const std::shared_ptr<ProcessName> &processInfo,
    const std::shared_ptr<Configuration> &configuration);

  /** Get the singleton logger service instance.
   * @return reference to the singleton LoggerService */
  static LoggerService &get();

  /** Initialize the singleton with the given parameters.
   * @param processInfo   the process name information
   * @param configuration the application configuration */
  static void initSingleton(const std::shared_ptr<ProcessName> &processInfo,
    const std::shared_ptr<Configuration> &configuration);

  /** Initialize the singleton a second time (re-init). */
  static void initSingleton2();

  /** Log a formatted message at the given level with source location.
   * @tparam Args the argument types
   * @param source  the source location (file, line, function)
   * @param lvl     the log level
   * @param message the format string
   * @param args    the format arguments */
  template<typename... Args>
  void log(spdlog::source_loc source, spdlog::level::level_enum lvl, spdlog::string_view_t message, Args &&...args)
  {
    m_logger->log(source, lvl, message, args...);
  }
};

#define DECLARE_LOGGER(processName, configuration) LoggerService::getOrCreate(processName, configuration)
#define LOG_INFO(message, ...) \
  LoggerService::get().log(    \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::info, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) \
  LoggerService::get().log(     \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::debug, message, ##__VA_ARGS__)
#define LOG_TRACE(message, ...) \
  LoggerService::get().log(     \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::trace, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) \
  LoggerService::get().log(     \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::err, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) \
  LoggerService::get().log(     \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::critical, message, ##_VA_ARGS__)
#define LOG_WARN(message, ...) \
  LoggerService::get().log(    \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::warn, message, ##__VA_ARGS__)
#endif// LOGGING_LOGGER_H
