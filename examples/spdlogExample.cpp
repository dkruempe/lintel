#include <iostream>
#include <libgen.h>
#include <map>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/tcp_sink.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <utility>

/*
 * LoggingService
 *
 * Aufgaben:
 * -> configuration von spdlog übernehmen bzw. funktionen bereitstellen
 * -> log funktion wrappen und weiterleiten
 * -> Sicherstellung das Fehler von fmt während der compiletime angezeigt werden
 * bzw. direkt in CLion selber
 * -> Support von mehreren Typen "Client, Server und Standalone"
 *    ** Standalone ist normale Variante
 *    ** Client/Server ist die Variante wo der Client alle log Nachrichten via
 * queue am Server leitet hierbei ist der unterschied zu asynchronen Variante,
 * dass es vollständig in ein anderen Prozess ausgelagert werden könnte
 */

class LoggingService {
private:
  std::string processName;
  std::shared_ptr<spdlog::logger> logger; // process Logger
  std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;
  static LoggingService *instance;
  static std::once_flag initInstanceFlag;

  /*
   * HELPER FUNCTIONS
   */
  static spdlog::sink_ptr
  changeLevelOrPatternOf(const spdlog::sink_ptr &sink,
                         spdlog::level::level_enum level,
                         const std::string &pattern) {
    if (!pattern.empty()) {
      sink->set_pattern(pattern);
    }
    sink->set_level(level);
    return sink;
  }

  static std::map<std::string, std::shared_ptr<spdlog::logger>>
  initiateLoggers(const std::vector<std::string> &loggerNames,
                  const std::vector<spdlog::sink_ptr> &sinks) {
    std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;
    std::transform(
        loggerNames.begin(), loggerNames.end(),
        std::inserter(loggers, loggers.begin()),
        [&](const std::string &loggerName)
            -> std::pair<std::string, std::shared_ptr<spdlog::logger>> {
          return {loggerName, std::make_shared<spdlog::logger>(
                                  loggerName, sinks.begin(), sinks.end())};
        });
    return loggers;
  }

public:
  LoggingService(const std::string &processName,
                 spdlog::level::level_enum defaultLogLevel,
                 const std::vector<spdlog::sink_ptr> &sinks,
                 const std::vector<std::string> &loggerNames)
      : processName(processName), logger(std::make_shared<spdlog::logger>(
                                      processName, sinks.begin(), sinks.end())),
        loggers(initiateLoggers(loggerNames, sinks)) {
    logger->set_level(defaultLogLevel);
    for (const auto &[name, logger] : loggers) {
      logger->set_level(defaultLogLevel);
    }
  }

  template <class... Args>
  void log(spdlog::level::level_enum level, spdlog::source_loc loc,
           spdlog::string_view_t message, const Args &... args) {
    logger->log(loc, level, message, args...);
  }

  template <class... Args>
  void log(const std::string &name, spdlog::level::level_enum level,
           spdlog::source_loc loc, spdlog::string_view_t message,
           const Args &... args) {
    auto found = loggers.find(name);
    if (found == loggers.end()) {
      // fallback
      logger->log(loc, level, message, args...);
      return;
    }
    found->second->log(loc, level, message, args...);
  }

  /*
   * Sink Creator Functions
   */
  static spdlog::sink_ptr createColourConsoleSink(
      spdlog::level::level_enum level = spdlog::level::trace,
      const std::string &pattern = "") {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    return changeLevelOrPatternOf(console_sink, level, pattern);
  }

  static spdlog::sink_ptr
  createTcpSink(const std::string &ipAddress, int port,
                spdlog::level::level_enum level = spdlog::level::trace,
                const std::string &pattern = "") {
    spdlog::sinks::tcp_sink_config config(ipAddress, port);
    auto tcpSink = std::make_shared<spdlog::sinks::tcp_sink_mt>(config);
    tcpSink->set_pattern(
        R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%fZ", "logger":"%n", "@severity":"%l", "file":"%s", "line":"%#", "message":"%v"})");
    return tcpSink;
  }

  // TODO create all other needed sinks

  /*
   * CONFIGURE FUNCTIONS
   */
  void configure(spdlog::level::level_enum level) { logger->set_level(level); }

  /*
   * Singleton FUNCTIONS
   */
  static LoggingService &
  getOrCreate(const std::string &processName,
              spdlog::level::level_enum defaultLogLevel,
              const std::vector<spdlog::sink_ptr> &sinks,
              const std::vector<std::string> &loggerNames) {
    if (!processName.empty()) {
      std::call_once(initInstanceFlag, &LoggingService::initSingleton,
                     processName, defaultLogLevel, sinks, loggerNames);
    }
    return get();
  }
  static LoggingService &get() {
    if (instance == nullptr) {
      throw std::runtime_error(
          "LoggingService not initialized. Please call Marco DECLARE_LOGGER");
    }
    return *instance;
  }
  static void initSingleton(const std::string &processName,
                            spdlog::level::level_enum defaultLogLevel,
                            const std::vector<spdlog::sink_ptr> &sinks,
                            const std::vector<std::string> &loggerNames) {
    instance =
        new LoggingService(processName, defaultLogLevel, sinks, loggerNames);
  }
};

#define DECLARE_LOGGER(processName, defaultLogLevel, sinks, loggerNames)       \
  LoggingService::getOrCreate(processName, defaultLogLevel, sinks, loggerNames)
#define LOG_INFO(message, ...)                                                 \
  LoggingService::get().log(                                                   \
      spdlog::level::info,                                                     \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_INFO_NAME(name, message, ...)                                      \
  LoggingService::get().log(                                                   \
      name, spdlog::level::info,                                               \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)                                                \
  LoggingService::get().log(                                                   \
      spdlog::level::debug,                                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_DEBUG_NAME(name, message, ...)                                     \
  LoggingService::get().log(                                                   \
      name, spdlog::level::debug,                                              \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_TRACE(message, ...)                                                \
  LoggingService::get().log(                                                   \
      spdlog::level::trace,                                                    \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_TRACE_NAME(name, message, ...)                                     \
  LoggingService::get().log(                                                   \
      name, spdlog::level::trace,                                              \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_ERROR(message, ...)                                                \
  LoggingService::get().log(                                                   \
      spdlog::level::err,                                                      \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_ERROR_NAME(name, message, ...)                                     \
  LoggingService::get().log(                                                   \
      name, spdlog::level::err,                                                \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_WARN(message, ...)                                                 \
  LoggingService::get().log(                                                   \
      spdlog::level::warn,                                                     \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
#define LOG_WARN_NAME(name, message, ...)                                      \
  LoggingService::get().log(                                                   \
      name, spdlog::level::warn,                                               \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, message,        \
      ##__VA_ARGS__)
LoggingService *LoggingService::instance = nullptr;
std::once_flag LoggingService::initInstanceFlag;

int main(int argc, char *argv[]) {
  std::vector<spdlog::sink_ptr> sinks = {
      LoggingService::createColourConsoleSink(spdlog::level::trace),
      LoggingService::createTcpSink("10.0.1.13", 4560)};
  std::vector<std::string> tasks;
  tasks.reserve(1000);
  for (int i = 0; i < 1000; i++) {
    tasks.emplace_back("Task" + std::to_string(i));
  }
  DECLARE_LOGGER(basename(argv[0]), spdlog::level::trace, sinks, tasks);
  std::cout << "Start Logging Test" << std::endl;
  LOG_TRACE("Hello, {}", "World");
  LOG_DEBUG("Hello, {}", "World");
  LOG_INFO("Hello, {}", "World");
  LOG_WARN("Hello, {}", "World");
  LOG_ERROR("Hello, {}", "World");

  std::this_thread::sleep_for(std::chrono::seconds(2));
  for (int i = 0; i < 20; i++) {
    LOG_DEBUG("Hello, {}", "World");
  }
  for (auto &task : tasks) {
    LOG_TRACE_NAME(task, "Hello, {}", "World");
    LOG_DEBUG_NAME(task, "Hello, {}", "World");
    LOG_INFO_NAME(task, "Hello, {}", "World");
    LOG_WARN_NAME(task, "Hello, {}", "World");
    LOG_ERROR_NAME(task, "Hello, {}", "World");
  }
  return 0;
}