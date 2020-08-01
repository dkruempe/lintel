#include <base_library/File.h>
#include <base_library/config.h>
#include <fmt/format.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/propertyconfigurator.h>
#include <map>
#include <vector>

class MultiLogger {
private:
  const std::string &name;
  File templateFile;
  std::vector<std::string> templateLogger;
  std::string rootLogger;
  File multiLoggerConfig;
  std::map<std::string, log4cxx::LoggerPtr> loggers;
  constexpr static std::string_view templateFileName = "template_log4cxx.cfg";

  // TODO extract layout to configuration
  log4cxx::LayoutPtr layoutPtr = new log4cxx::PatternLayout(
      "[%d{yyyy-MM-dd HH:mm:ss}|T%t|%-5p|%C::%M(%L)]: %m -- %l%n");

  void configure() {
    std::for_each(loggers.begin(), loggers.end(),
                  [&](const std::pair<std::string, log4cxx::LoggerPtr> &iter) {
                    log4cxx::FileAppenderPtr fileAppender =
                        new log4cxx::FileAppender(
                            layoutPtr, std::string(LOG_DIRECTORY) + "/" +
                                           iter.first + ".log");
                    log4cxx::ConsoleAppenderPtr consoleAppender =
                        new log4cxx::ConsoleAppender(layoutPtr);
                    iter.second->addAppender(fileAppender);
                    iter.second->addAppender(consoleAppender);
                    iter.second->setLevel(log4cxx::Level::toLevel("INFO"));
                  });
  }

public:
  explicit MultiLogger(const std::string &name,
                       const std::vector<std::string> &loggerNames)
      : name(name), templateFile(std::string(CONFIG_DIRECTORY) + "/" +
                                 std::string(templateFileName)),
        templateLogger(templateFile.matches(std::regex(".*\\.template.*"))),
        rootLogger(templateFile.matches(std::regex(".*log4j.rootLogger.*"))[0]),
        multiLoggerConfig(std::string(CONFIG_DIRECTORY) + "/" + name +
                          "_log4cxx.cfg") {
    for (const auto &loggerName : loggerNames) {
      loggers.insert({loggerName, log4cxx::Logger::getLogger(loggerName)});
    }
    configure();
  }

  void changeLevelOf(const std::string &name, const std::string &level) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->setLevel(log4cxx::Level::toLevel(level));
  }

  template <class... Args>
  void debug(const std::string &name, const std::string &function,
             const std::string &file, int line, const std::string &message,
             Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->debug(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void info(const std::string &name, const std::string &function,
            const std::string &file, int line, const std::string &message,
            Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->info(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void trace(const std::string &name, const std::string &function,
            const std::string &file, int line, const std::string &message,
            Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->trace(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void fatal(const std::string &name, const std::string &function,
            const std::string &file, int line, const std::string &message,
            Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->fatal(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void error(const std::string &name, const std::string &function,
            const std::string &file, int line, const std::string &message,
            Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->error(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }

  template <class... Args>
  void warn(const std::string &name, const std::string &function,
            const std::string &file, int line, const std::string &message,
            Args... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      return;
    }

    found->second->warn(
        fmt::format(message, args...),
        log4cxx::spi::LocationInfo(file.c_str(), function.c_str(), line));
  }
};
#define LOG_WARN(message, ...)                                                 \
  Logger::get().warn(__LOG4CXX_FUNC__, __FILE__, __LINE__, message,            \
                     ##__VA_ARGS__)

#define MULTI_LOG_DEBUG(name, message, ...)                                    \
  multiLogger.debug(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,       \
                    ##__VA_ARGS__);
#define MULTI_LOG_DEBUG_DETAIL(name, func, file, line, message, ...)           \
  multiLogger.debug(name, func, file, line, message, ##__VA_ARGS__);
#define MULTI_LOG_INFO(name, message, ...)                                     \
  multiLogger.info(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,        \
                   ##__VA_ARGS__);
#define MULTI_LOG_INFO_DETAIL(name, func, file, line, message, ...)            \
  multiLogger.info(name, func, file, line, message, ##__VA_ARGS__);
#define MULTI_LOG_TRACE(name, message, ...)                                     \
  multiLogger.trace(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,        \
                   ##__VA_ARGS__);
#define MULTI_LOG_TRACE_DETAIL(name, func, file, line, message, ...)            \
  multiLogger.trace(name, func, file, line, message, ##__VA_ARGS__);
#define MULTI_LOG_ERROR(name, message, ...)                                     \
  multiLogger.error(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,        \
                   ##__VA_ARGS__);
#define MULTI_LOG_ERROR_DETAIL(name, func, file, line, message, ...)            \
  multiLogger.error(name, func, file, line, message, ##__VA_ARGS__);
#define MULTI_LOG_FATAL(name, message, ...)                                     \
  multiLogger.fatal(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,        \
                   ##__VA_ARGS__);
#define MULTI_LOG_FATAL_DETAIL(name, func, file, line, message, ...)            \
  multiLogger.fatal(name, func, file, line, message, ##__VA_ARGS__);
#define MULTI_LOG_WARN(name, message, ...)                                     \
  multiLogger.warn(name, __LOG4CXX_FUNC__, __FILE__, __LINE__, message,        \
                   ##__VA_ARGS__);
#define MULTI_LOG_WARN_DETAIL(name, func, file, line, message, ...)            \
  multiLogger.warn(name, func, file, line, message, ##__VA_ARGS__);

int main(int argc, char *argv[]) {
  std::vector<std::string> loggerNames = {"anna", "example_user"};
  MultiLogger multiLogger("multilogger", loggerNames);
  multiLogger.changeLevelOf("anna", "ALL");
  MULTI_LOG_DEBUG("anna", "test")
  MULTI_LOG_DEBUG("example_user", "test2")
  MULTI_LOG_DEBUG_DETAIL("example_user", __LOG4CXX_FUNC__, __FILE__, __LINE__,
                         "detail multi debug log");
  MULTI_LOG_ERROR("anna", "error")
  MULTI_LOG_ERROR("example_user", "error")
  MULTI_LOG_TRACE("anna", "trace")
  MULTI_LOG_TRACE("example_user", "trace")
  MULTI_LOG_INFO("anna", "info")
  MULTI_LOG_INFO("example_user", "info")
  MULTI_LOG_FATAL("anna", "fatal")
  MULTI_LOG_FATAL("example_user", "fatal")
  MULTI_LOG_WARN("anna", "warn")
  MULTI_LOG_WARN("example_user", "warn");
  return 0;
}