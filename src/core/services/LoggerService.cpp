#include "lintel/core/services/LoggerService.h"

#include "lintel/core/configuration/Configuration.h"
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/tcp_sink.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <iostream>
#include <regex>

#include "lintel/config.h"
#include "lintel/core/configuration/LoggerComponent.h"
#include "lintel/core/configuration/LoggerEntry.h"
#include "lintel/core/exceptions/LoggerServiceNotInitialized.h"
#include "lintel/core/services/DirectoryService.h"
#include "lintel/core/services/FileService.h"
#include "lintel/core/utils/UUID.h"

// spdlog is used here only, never in the public header
#include <spdlog/logger.h>

// initialization of static variables
namespace lintel {
std::unique_ptr<LoggerService> LoggerService::m_instance = nullptr;
std::mutex LoggerService::m_instanceMutex;

LoggerService::LoggerService()
  : m_processName(nullptr), m_configuration(nullptr),
    // A non-registered logger on purpose: spdlog::stdout_color_mt() would park the sink in
    // spdlog's global registry for the whole process lifetime and show up as a leak (P4) in the
    // LeakSanitizer job. This fallback logger is only used until DECLARE_LOGGER replaces it.
    m_logger(std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::stdout_color_sink_mt>()))
{}

LoggerService::LoggerService(std::shared_ptr<ProcessName> processName, std::shared_ptr<Configuration> configuration)
  : m_processName(std::move(processName)), m_configuration(std::move(configuration)), m_logger(init())
{}

namespace {
  void setSinkLevelPattern(const LoggerSinkConfiguration &config, const spdlog::sink_ptr &sinkPtr)
  {
    // one parser for every level source (LoggerService::logLevelFrom), so a sink
    // honours the same names as the logger and the admin endpoint - among them
    // the "error" alias that the former strcmp chain dropped. An unknown name
    // leaves the sink at its default level, exactly as before.
    if (const std::optional<LogLevel> level = LoggerService::logLevelFrom(config.getLevel()); level.has_value()) {
      // LogLevel intentionally has the values of spdlog::level::level_enum
      sinkPtr->set_level(static_cast<spdlog::level::level_enum>(*level));
    }
    const std::string &pattern = config.getPattern();
    if (!pattern.empty()) { sinkPtr->set_pattern(config.getPattern()); }
  }

  std::filesystem::path resolveSinkPath(const std::shared_ptr<LoggerPathConfiguration> &pathConfig,
    const std::string &processName)
  {
    auto path = pathConfig->getPath();
    if (pathConfig->isCreateSubDirectories()) { path /= processName; }
    if (is_regular_file(path)) { throw std::runtime_error("path is file and not directory"); }
    if (!exists(path)) { create_directories(path); }
    return path;
  }

  std::filesystem::path resolveFilePath(const std::filesystem::path &basePath,
    const LoggerSinkConfiguration &config,
    const std::string &processName)
  {
    if (config.getFileName().empty()) { return basePath / (processName + ".log"); }
    return basePath / (config.getFileName() + ".log");
  }

  void collectSinks(std::vector<spdlog::sink_ptr> &sinks,
    const std::shared_ptr<LoggerConfiguration> &logConfig,
    const std::shared_ptr<LoggerEntry> &loggerPathEntry,
    const std::string &processName)
  {
    for (const auto &logSinkConfig : logConfig->getLoggerSinks()) {
      switch (logSinkConfig.getType()) {
      case LoggerSinkConfiguration::SysLogSink:
        break;
      case LoggerSinkConfiguration::ConsoleSink: {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        setSinkLevelPattern(logSinkConfig, consoleSink);
        sinks.push_back(consoleSink);
        break;
      }
      case LoggerSinkConfiguration::RotatingFileSink: {
        auto basePath = resolveSinkPath(loggerPathEntry->getLoggerPathConfiguration(), processName);
        auto filePath = resolveFilePath(basePath, logSinkConfig, processName);
        auto rotatingFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          filePath.string(), logSinkConfig.getFileSize(), logSinkConfig.getMaxFiles());
        setSinkLevelPattern(logSinkConfig, rotatingFileSink);
        sinks.push_back(rotatingFileSink);
        break;
      }
      case LoggerSinkConfiguration::DailyFileSink: {
        const std::string &time = logSinkConfig.getTime();
        if (time.length() != 5) { throw std::runtime_error("wrong configuration of time"); }
        int32_t hour = std::stoi(time.substr(0, 2));
        int32_t minute = std::stoi(time.substr(3, 2));
        auto basePath = resolveSinkPath(loggerPathEntry->getLoggerPathConfiguration(), processName);
        auto filePath = resolveFilePath(basePath, logSinkConfig, processName);
        auto dailyFileSink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(filePath.string(), hour, minute);
        setSinkLevelPattern(logSinkConfig, dailyFileSink);
        sinks.push_back(dailyFileSink);
        break;
      }
      case LoggerSinkConfiguration::TcpSink: {
        spdlog::sinks::tcp_sink_config tcpSinkConfig(logSinkConfig.getConnection(), logSinkConfig.getPort());
        auto tcpSink = std::make_shared<spdlog::sinks::tcp_sink_mt>(tcpSinkConfig);
        setSinkLevelPattern(logSinkConfig, tcpSink);
        sinks.push_back(tcpSink);
        break;
      }
      }
    }
  }

  void applyLoggerLevel(const std::shared_ptr<spdlog::logger> &logger, const std::string &loggerLevel)
  {
    // one parser for every level source (LoggerService::logLevelFrom): "error"
    // and "err" are the same level here as in configuredLevelOf()/levelOf(), so
    // the level the configuration reports is also the one the logger runs on.
    // An unknown name keeps the spdlog defaults (level info), which is exactly
    // what configuredLevelOf() reports for it.
    const std::optional<LogLevel> level = LoggerService::logLevelFrom(loggerLevel);
    if (!level.has_value()) { return; }
    // LogLevel intentionally has the values of spdlog::level::level_enum
    const auto spdlogLevel = static_cast<spdlog::level::level_enum>(*level);
    logger->set_level(spdlogLevel);
    logger->flush_on(spdlogLevel);
  }

  /** The request/correlation id of the calling thread.
   *
   * thread_local because an httplib worker thread handles one request at a time: the
   * value is written once per request (see RequestIdGuard) and every LOG_* call in the
   * handler picks it up without an explicit parameter. */
  thread_local std::string g_requestId;

  /** Longest accepted client-supplied correlation id (see requestIdOf). */
  constexpr std::size_t kMaxRequestIdLength = 64;

  /** The configured level name as an enum; an unknown name stays at info,
   *  because a typo in the XML must not keep the process from starting. */
  LogLevel configuredLevelOf(const std::string &name)
  {
    return LoggerService::logLevelFrom(name).value_or(LogLevel::info);
  }
}// namespace

std::shared_ptr<spdlog::logger> LoggerService::init()
{
  std::vector<spdlog::sink_ptr> sinks;
  auto entries = m_configuration->configurationOf<LoggerComponent>();
  auto loggerPath = std::find_if(entries.begin(), entries.end(), [](auto entry) {
    std::shared_ptr<LoggerEntry> loggerEntry = std::static_pointer_cast<LoggerEntry>(entry);
    return loggerEntry->isLoggerPathConfiguration();
  });
  if (loggerPath == entries.end()) { throw std::runtime_error("Please configure logger path"); }
  auto loggerConfiguration = std::find_if(entries.begin(), entries.end(), [&](auto entry) {
    std::shared_ptr<LoggerEntry> loggerEntry = std::static_pointer_cast<LoggerEntry>(entry);
    std::shared_ptr<LoggerConfiguration> loggerCfg = loggerEntry->getLoggerConfiguration();
    if (loggerCfg == nullptr) { return false; }
    std::regex regex(loggerCfg->getProcessName());
    return loggerCfg->getProcessName() == m_processName->getProcessName()
           || std::regex_match(m_processName->getProcessName(), regex);
  });
  if (loggerConfiguration == entries.end()) { throw std::runtime_error("No matching Logger Configuration found"); }
  auto loggerPathEntry = std::static_pointer_cast<LoggerEntry>(*loggerPath);
  auto loggerConfigEntry = std::static_pointer_cast<LoggerEntry>(*loggerConfiguration);
  std::shared_ptr<LoggerConfiguration> logConfig = loggerConfigEntry->getLoggerConfiguration();
  collectSinks(sinks, logConfig, loggerPathEntry, m_processName->getProcessName());
  auto logger = std::make_shared<spdlog::logger>(m_processName->getProcessName(), sinks.begin(), sinks.end());
  std::string loggerLevel = loggerConfigEntry->getLoggerConfiguration()->getLevel();
  applyLoggerLevel(logger, loggerLevel);
  // keep the configured level visible to levelOf()/setLevel(), even when the
  // configuration names an unknown level and applyLoggerLevel stays silent
  m_level.store(configuredLevelOf(loggerLevel), std::memory_order_relaxed);
  // A pattern on the logger wins over a per-sink pattern (spdlog applies it to
  // every attached sink), which is what a JSON log wants: the format belongs to
  // the logger, not to the destination. %v is inserted verbatim, so callers and
  // the request id must not carry quotes (see requestIdOf()).
  if (logConfig->isJson() && logConfig->getPattern().empty()) {
    logger->set_pattern("{\"time\":\"%Y-%m-%dT%H:%M:%S.%e%z\",\"name\":\"%n\",\"level\":\"%l\",\"msg\":\"%v\"}");
  } else if (!logConfig->getPattern().empty()) {
    logger->set_pattern(logConfig->getPattern());
  }
  return logger;
}

LoggerService::~LoggerService() = default;

void LoggerService::setLevel(LogLevel level)
{
  m_level.store(level, std::memory_order_relaxed);
  if (m_logger == nullptr) { return; }
  const auto spdlogLevel = static_cast<spdlog::level::level_enum>(static_cast<std::uint8_t>(level));
  m_logger->set_level(spdlogLevel);
  m_logger->flush_on(spdlogLevel);
}

LogLevel LoggerService::levelOf() const { return m_level.load(std::memory_order_relaxed); }

std::optional<LogLevel> LoggerService::logLevelFrom(std::string_view name) noexcept
{
  if (name == "critical") { return LogLevel::critical; }
  if (name == "warn") { return LogLevel::warn; }
  if (name == "err" || name == "error") { return LogLevel::error; }
  if (name == "info") { return LogLevel::info; }
  if (name == "debug") { return LogLevel::debug; }
  if (name == "trace") { return LogLevel::trace; }
  if (name == "off") { return LogLevel::off; }
  return std::nullopt;
}

std::string LoggerService::requestIdOf(const std::string &candidate)
{
  // keep the documented id alphabet; anything else is replaced instead of
  // escaped, because the id is echoed back and lands in unescaped JSON
  if (candidate.empty() || candidate.size() > kMaxRequestIdLength) { return generateRequestId(); }
  const bool valid = std::all_of(candidate.begin(), candidate.end(), [](unsigned char character) {
    return std::isalnum(character) != 0 || character == '.' || character == '_' || character == '-';
  });
  return valid ? candidate : generateRequestId();
}

std::string LoggerService::generateRequestId() { return UUID::generate(); }

void LoggerService::setRequestId(std::string requestId) { g_requestId = std::move(requestId); }

std::string_view LoggerService::requestId() { return g_requestId; }

LoggerService &LoggerService::getOrCreate(const std::shared_ptr<ProcessName> &processInfo,
  const std::shared_ptr<Configuration> &configuration)
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  // replace a default instance that may have been created by an early
  // LOG_* call before DECLARE_LOGGER, so the configured logger wins
  m_instance = std::make_unique<LoggerService>(processInfo, configuration);
  return *m_instance;
}

LoggerService &LoggerService::get()
{
  std::lock_guard<std::mutex> lock(m_instanceMutex);
  if (m_instance == nullptr) { m_instance = std::make_unique<LoggerService>(); }
  return *m_instance;
}

void LoggerService::initSingleton(const std::shared_ptr<ProcessName> &processInfo,
  const std::shared_ptr<Configuration> &configuration)
{
  m_instance = std::make_unique<LoggerService>(processInfo, configuration);
}

void LoggerService::initSingleton2() { m_instance = std::make_unique<LoggerService>(); }

void LoggerService::logImpl(const std::source_location &location,
  LogLevel lvl,
  std::string_view message,
  std::format_args args)
{
  if (m_logger == nullptr) { return; }
  auto level = static_cast<spdlog::level::level_enum>(lvl);
  if (!m_logger->should_log(level)) { return; }
  std::string text;
  try {
    text = std::vformat(message, args);
  } catch (const std::exception &ex) {
    text = std::format("log formatting failed: {} [{}:{}]", ex.what(), location.file_name(), location.line());
  }
  const spdlog::source_loc source{ location.file_name(), static_cast<int>(location.line()), location.function_name() };
  if (g_requestId.empty()) {
    m_logger->log(source, level, std::string_view(text));
    return;
  }
  // the correlation id of the request the calling thread is serving: it ties the log lines
  // of one request together and matches the X-Request-Id header the server answered with
  text = std::format("[{}] {}", g_requestId, text);
  m_logger->log(source, level, std::string_view(text));
}
}// namespace lintel