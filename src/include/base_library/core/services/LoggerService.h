#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * LoggerService class for easier logging in process itself
 */
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <log4cxx/spi/loggingevent.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

class LoggerService {
 private:
  // constexpr constants
  constexpr static std::string_view m_templateConfig = "template_log4cxx.xml";
  constexpr static std::string_view m_templateMarker = "template.log";

  // variables
  const std::string m_processName;
  log4cxx::LoggerPtr m_logger;

  void configure(bool consoleOnly);

  static LoggerService *m_instance;
  static std::once_flag m_initInstanceFlag;

 public:
  explicit LoggerService(const std::string &processName);

  explicit LoggerService();

  ~LoggerService();

  static LoggerService &getOrCreate(const std::string &argv);
  static LoggerService &get();
  static void initSingleton(const std::string &processName);
  static void init();

  template <class... Args>
  void info(const std::string &function, const std::string &file, int line,
            const std::string &message, Args... args) {
    m_logger->info(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void debug(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    m_logger->debug(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void trace(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    m_logger->trace(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void error(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    m_logger->error(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void fatal(const std::string &function, const std::string &file, int line,
             const std::string &message, Args... args) {
    m_logger->fatal(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void warn(const std::string &function, const std::string &file, int line,
            const std::string &message, Args... args) {
    m_logger->warn(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }
};
#define DECLARE_LOGGER(argv) LoggerService::getOrCreate(argv)
#define LOG_INFO(message, ...)                                             \
  LoggerService::get().info(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                            ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)                                             \
  LoggerService::get().debug(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                             ##__VA_ARGS__)
#define LOG_TRACE(message, ...)                                             \
  LoggerService::get().trace(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                             ##__VA_ARGS__)
#define LOG_ERROR(message, ...)                                             \
  LoggerService::get().error(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                             ##__VA_ARGS__)
#define LOG_FATAL(message, ...)                                             \
  LoggerService::get().fatal(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                             ##__VA_ARGS__)
#define LOG_WARN(message, ...)                                             \
  LoggerService::get().warn(__LOG4CXX_FUNC__, __FILE__, __LINE__, message, \
                            ##__VA_ARGS__)
#endif  // LOGGING_LOGGER_H
