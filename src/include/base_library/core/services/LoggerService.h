#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 */
#include <spdlog/spdlog.h>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/models/Process.h"

class LoggerService {
 private:
  // variables
  std::shared_ptr<Process::ProcessInfo> m_processInfo;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<spdlog::logger> m_logger;

  std::shared_ptr<spdlog::logger> init();

  static LoggerService *m_instance;
  static std::once_flag m_initInstanceFlag;

 public:
  LoggerService(std::shared_ptr<Process::ProcessInfo> processInfo,
                std::shared_ptr<Configuration> configuration);

  LoggerService() = delete;

  ~LoggerService();

  static LoggerService &getOrCreate(
      const std::shared_ptr<Process::ProcessInfo> &processInfo,
      const std::shared_ptr<Configuration> &configuration);

  static LoggerService &get();
  static void initSingleton(
      const std::shared_ptr<Process::ProcessInfo> &processInfo,
      const std::shared_ptr<Configuration> &configuration);

  template <typename... Args>
  void log(spdlog::source_loc source, spdlog::level::level_enum lvl,
           spdlog::string_view_t message, Args &&...args) {
    m_logger->log(source, lvl, message, args...);
  }
};
#define DECLARE_LOGGER(processInfo, configuration) \
  LoggerService::getOrCreate(processInfo, configuration)
#define LOG_INFO(message, ...)                                 \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::info, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)                                \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::debug, message, ##__VA_ARGS__)
#define LOG_TRACE(message, ...)                                \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::trace, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...)                                \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::err, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...)                                \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::critical, message, ##_VA_ARGS__)
#define LOG_WARN(message, ...)                                 \
  LoggerService::get().log(                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
      spdlog::level::warn, message, ##__VA_ARGS__)
#endif  // LOGGING_LOGGER_H
