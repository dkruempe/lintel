#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 *
 * This public header parses only &lt;format&gt;. spdlog is hidden behind a
 * forward declaration and stays in LoggerService.cpp.
 * The variadic log() applies std::format's template-elimination idea
 * (std::format -> std::vformat): the template only boxes its arguments with
 * std::make_format_args and forwards them to the non-template logImpl(), so
 * formatting and spdlog are compiled once instead of at every call site.
 */
#include <atomic>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

namespace lintel {
class Configuration;
}// namespace lintel
#include "lintel/core/models/ProcessName.h"

namespace spdlog {
/** Forward declaration only; users of this header never need spdlog itself. */
class logger;
}// namespace spdlog

namespace lintel {
/**
 * Log levels used by LoggerService and the LOG_* macros.
 * The values intentionally match spdlog::level::level_enum.
 */
enum class LogLevel : std::uint8_t { trace = 0, debug = 1, info = 2, warn = 3, error = 4, critical = 5, off = 6 };

/** Singleton service providing spdlog-based logging for the application.
 *
 * JSON output (`json="true"` on `<Logger>` in the bootstrap XML) formats every
 * message as one JSON object per line with the message text as `msg`. That text
 * is inserted verbatim, it is **not** escaped: a message containing a `"` or `\`
 * (or a control character) ends or breaks the JSON line, so callers must keep
 * structured data out of quotes. The request/correlation id is the only
 * component that is filtered before it is written (see requestIdOf()). */
class LoggerService
{
private:
  // variables
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<Configuration> m_configuration;
  /** Effective log level, kept so that levelOf() survives a runtime change.
   *  Declared before m_logger on purpose: init() writes it while m_logger is
   *  being constructed, and members are initialized in declaration order. */
  std::atomic<LogLevel> m_level{ LogLevel::info };
  std::shared_ptr<spdlog::logger> m_logger;

  /** Initialize the spdlog logger instance. */
  std::shared_ptr<spdlog::logger> init();

  /**
   * Non-template formatting backend: filters the level and performs all fmt formatting.
   * spdlog is touched here only. Defined in LoggerService.cpp.
   */
  void logImpl(const std::source_location &location, LogLevel lvl, std::string_view message, std::format_args args);

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
   * Template-eliminated: boxes the arguments (std::make_format_args) and defers all formatting
   * to the non-template logImpl() in LoggerService.cpp.
   * @tparam Args the argument types
   * @param location the source location of the log statement
   * @param lvl      the log level
   * @param message  the format string
   * @param args     the format arguments */
  template<typename... Args>
  void log(const std::source_location &location, LogLevel lvl, std::string_view message, Args &&...args)
  {
    logImpl(location, lvl, message, std::make_format_args(args...));
  }

  /** Change the log level at runtime.
   *
   * Thread-safe: spdlog keeps its level in an atomic, so a running process can be
   * re-diagnosed without a restart (see the /admin/log-level endpoint).
   * @param level the new level */
  void setLevel(LogLevel level);

  /** @return the currently effective log level */
  [[nodiscard]] LogLevel levelOf() const;

  /** Parse a configured level name.
   *
   * The single parser for both the XML configuration and the admin endpoint:
   * the endpoint has to reject an unknown name, the configuration silently falls
   * back to info - the caller decides with the returned optional.
   *
   * @param name critical|warn|err|error|info|debug|trace|off
   * @return the level, std::nullopt for an unknown name */
  static std::optional<LogLevel> logLevelFrom(std::string_view name) noexcept;

  /** Validate a client supplied correlation id.
   *
   * The id ends up in every log line of the request, and the JSON format does
   * not escape it, so an unchecked header could break the JSON of a line (and
   * inject log content). Anything outside [A-Za-z0-9._-] or longer than 64
   * characters is replaced by a freshly generated id.
   *
   * @param candidate the value of the X-Request-Id header
   * @return the candidate when it is safe to log, a new id otherwise */
  static std::string requestIdOf(const std::string &candidate);

  /** Generate an opaque correlation id for one request. */
  static std::string generateRequestId();

  /** Set the request/correlation id of the calling thread.
   * @param requestId the id, empty to clear */
  static void setRequestId(std::string requestId);

  /** @return the request/correlation id of the calling thread, empty when unset */
  static std::string_view requestId();
};

/** RAII guard that keeps a request/correlation id on the calling thread.
 *
 * Restores the previous id so that a reused httplib worker thread cannot leak the
 * correlation of one request into the next. */
class RequestIdGuard
{
private:
  std::string m_previous;

public:
  /** @param requestId the request id of the guarded scope */
  explicit RequestIdGuard(const std::string &requestId) : m_previous(LoggerService::requestId())
  {
    LoggerService::setRequestId(requestId);
  }

  ~RequestIdGuard() { LoggerService::setRequestId(m_previous); }

  RequestIdGuard(const RequestIdGuard &) = delete;
  RequestIdGuard &operator=(const RequestIdGuard &) = delete;
};

#define DECLARE_LOGGER(processName, configuration) lintel::LoggerService::getOrCreate(processName, configuration)
#define LOG_INFO(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::info, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::debug, message, ##__VA_ARGS__)
#define LOG_TRACE(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::trace, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::error, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::critical, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::warn, message, ##__VA_ARGS__)
}// namespace lintel
#endif// LOGGING_LOGGER_H
