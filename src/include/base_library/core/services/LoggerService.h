#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 *
 * This public header parses only <fmt/base.h>. spdlog is hidden behind a
 * forward declaration and stays in LoggerService.cpp.
 * The variadic log() applies fmt's template-elimination idea
 * (fmt::format -> fmt::vformat): the template only boxes its arguments with
 * fmt::make_format_args and forwards them to the non-template logImpl(), so
 * formatting and spdlog are compiled once instead of at every call site.
 */
#include <fmt/base.h>

#include <memory>
#include <mutex>

class Configuration;
#include "base_library/features/base/models/ProcessName.h"

namespace spdlog {
/** Forward declaration only; users of this header never need spdlog itself. */
class logger;
}// namespace spdlog

/**
 * Log levels used by LoggerService and the LOG_* macros.
 * The values intentionally match spdlog::level::level_enum.
 */
enum class LogLevel : int { trace = 0, debug = 1, info = 2, warn = 3, error = 4, critical = 5, off = 6 };

/** Source location function name for the LOG_* macros, equivalent to spdlog's SPDLOG_FUNCTION. */
#ifndef BASE_LIBRARY_FUNCTION
#if defined(_MSC_VER)
#define BASE_LIBRARY_FUNCTION __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#define BASE_LIBRARY_FUNCTION __PRETTY_FUNCTION__
#else
#define BASE_LIBRARY_FUNCTION __func__
#endif
#endif

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

  /**
   * Non-template formatting backend: filters the level and performs all fmt formatting.
   * spdlog is touched here only. Defined in LoggerService.cpp.
   */
  void logImpl(const char *file,
    int line,
    const char *function,
    LogLevel lvl,
    fmt::string_view message,
    fmt::format_args args);

  static std::unique_ptr<LoggerService> m_instance;
  static std::mutex m_instanceMutex;

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
   * Template-eliminated: boxes the arguments (fmt::make_format_args) and defers all formatting
   * to the non-template logImpl() in LoggerService.cpp.
   * @tparam Args the argument types
   * @param file     the source file name
   * @param line     the source line number
   * @param function the source function name
   * @param lvl      the log level
   * @param message  the format string
   * @param args     the format arguments */
  template<typename... Args>
  void log(const char *file, int line, const char *function, LogLevel lvl, fmt::string_view message, Args &&...args)
  {
    logImpl(file, line, function, lvl, message, fmt::make_format_args(args...));
  }
};

#define DECLARE_LOGGER(processName, configuration) LoggerService::getOrCreate(processName, configuration)
#define LOG_INFO(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::info, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::debug, message, ##__VA_ARGS__)
#define LOG_TRACE(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::trace, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::error, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::critical, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) \
  LoggerService::get().log(__FILE__, __LINE__, BASE_LIBRARY_FUNCTION, LogLevel::warn, message, ##__VA_ARGS__)
#endif// LOGGING_LOGGER_H
