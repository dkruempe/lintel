#ifndef CPP_BASE_LIBRARY_LOGGERMACROS_H
#define CPP_BASE_LIBRARY_LOGGERMACROS_H

/**
 * Plain (non-module) header containing the logging convenience macros.
 * Macros cannot be exported from C++20 modules, so they live in a separate
 * header that both module interface units and module consumers can include.
 */
#include <spdlog/logger.h>

class LoggerService;

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
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::critical, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) \
  LoggerService::get().log(    \
    spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::warn, message, ##__VA_ARGS__)

#endif// CPP_BASE_LIBRARY_LOGGERMACROS_H
