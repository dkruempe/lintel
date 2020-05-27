#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

/**
 * Logger class for easier logging in process itself
 */
#include "Scheduler.h"
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

class Logger {
private:
  // constexpr constants
  constexpr static std::string_view templateConfig = "template_log4cxx.xml";
  constexpr static std::string_view templateMarker = "template.log";
  Scheduler scheduler = Scheduler(1);

  // variables
  const std::string &processName;
  struct LOG_INFO {
    uint32_t logCount;
    std::chrono::steady_clock::time_point timePoint;
    std::string function;
    std::string file;
    int line;
    std::string logString;
    LOG_INFO(std::string function, std::string file, int line, std::string logString)
        : logCount(1), timePoint(std::chrono::steady_clock::now()),
          function(std::move(function)), file(std::move(file)), line(line), logString(std::move(logString)) {}
  };
  std::mutex mutex;
  std::map<std::string, LOG_INFO> repeatLogs;
  log4cxx::LoggerPtr logger;

  void configure();
  static std::string readFile(const std::filesystem::path &path);

  static void writeToFile(const std::filesystem::path &path,
                          const std::string &content);

  static Logger *instance;
  static std::once_flag initInstanceFlag;

  void logRepeatLog();

public:
  explicit Logger(const std::string &processName);
  ~Logger();

  static Logger &getOrCreate(const std::string &processName);
  static Logger &get();
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
  void repeat(const std::string &function, const std::string &file, int line,
              const std::string &message, Args... args) {
    std::string logString = fmt::format(message, args...);
    std::string key = logString + "_" + function + "_" + file + "_" + std::to_string(line);
    auto found = repeatLogs.find(key);
    if (found == repeatLogs.end()) {
      repeatLogs.insert({key, LOG_INFO(function, file, line, logString)});
    }
    (*found).second.logCount = (*found).second.logCount + 1;
  }

  template <class... Args>
  void warn(const std::string &function, const std::string &file, int line,
            const std::string &message, Args... args) {
    logger->warn(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }
};
#define DECLARE_LOGGER(processName) Logger::getOrCreate(processName)
#define LOG_INFO(message, ...)                                                 \
  Logger::get().info(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,            \
                     ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)                                                \
  Logger::get().debug(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_TRACE(message, ...)                                                \
  Logger::get().trace(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_ERROR(message, ...)                                                \
  Logger::get().error(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_FATAL(message, ...)                                                \
  Logger::get().fatal(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,           \
                      ##__VA_ARGS__)
#define LOG_WARN(message, ...)                                                 \
  Logger::get().warn(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,            \
                     ##__VA_ARGS__)
#define LOG_REPEAT(message, ...)                                               \
  Logger::get().repeat(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,          \
                       ##__VA_ARGS__)

#endif // LOGGING_LOGGER_H
