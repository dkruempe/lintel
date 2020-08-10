#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 */
#include "base_library/services/SchedulerService.h"
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <log4cxx/spi/loggingevent.h>
#include <map>
#include <mutex>
#include <thread>

class LoggerService {
private:
  // constexpr constants
  constexpr static std::string_view templateConfig = "template_log4cxx.xml";
  constexpr static std::string_view templateMarker = "template.log";

  // variables
  const std::string &processName;
  log4cxx::LoggerPtr logger;

  void configure();

  static LoggerService *instance;
  static std::once_flag initInstanceFlag;

public:
  explicit LoggerService(const std::string &processName);
  ~LoggerService();

  static LoggerService &getOrCreate(const std::string &processName);
  static LoggerService &get();
  static void initSingleton(const std::string &processName);

  template <class... Args>
  void info(const std::string &function, const std::string &file, int line,
            const std::string &message, Args... args) {
    logger->info(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void debug(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    logger->debug(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void trace(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    logger->trace(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void error(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    logger->error(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void fatal(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    logger->fatal(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void warn(const std::string &function, const std::string &file, int line,
            const std::string &message, Args... args) {
    logger->warn(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }
};
#define DECLARE_LOGGER(processName) LoggerService::getOrCreate(processName)
#define LOG_INFO(message, ...)                                                 \
  LoggerService::get().info(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,            \
                     ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)                                                \
  LoggerService::get().debug(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_TRACE(message, ...)                                                \
  LoggerService::get().trace(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_ERROR(message, ...)                                                \
  LoggerService::get().error(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_FATAL(message, ...)                                                \
  LoggerService::get().fatal(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_WARN(message, ...)                                                 \
  LoggerService::get().warn(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,            \
                     ##__VA_ARGS__)
#endif // LOGGING_LOGGER_H
