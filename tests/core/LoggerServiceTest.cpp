#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/StringUtils.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/base/configuration/LoggerComponent.h"
#include "lintel/features/base/configuration/LoggerConfiguration.h"
#include "lintel/features/base/configuration/LoggerEntry.h"
#include "lintel/features/base/configuration/LoggerPathConfiguration.h"
#include "lintel/features/base/configuration/LoggerSinkConfiguration.h"
#include "lintel/features/base/models/ProcessName.h"

#include <catch2/catch_all.hpp>

/**
 * LoggerService builds one spdlog logger per process from the LoggerComponent configuration.
 *
 * Covered: sink construction, the file name resolution including the empty file name branch of
 * resolveFilePath(), the level filtering of logger and of sink, the pattern, the format failure
 * fallback of logImpl(), the size based rotation and the singleton dispatch of LOG_*.
 * The SysLog sink is only covered through its "skipped" branch, the Tcp sink is not covered at
 * all, because it would need a listening server.
 */

namespace {

/** Temporary log directory that removes itself again. */
struct LoggerServiceLogDir
{
  std::filesystem::path path;

  LoggerServiceLogDir()
  {
    static unsigned int counter = 0;
    path = std::filesystem::temp_directory_path()
           / ("logger_service_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + "_log");
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
  }

  ~LoggerServiceLogDir() { std::filesystem::remove_all(path); }

  LoggerServiceLogDir(const LoggerServiceLogDir &) = delete;
  LoggerServiceLogDir &operator=(const LoggerServiceLogDir &) = delete;
};

/** @return a configuration with a logger path entry and one logger configuration */
std::shared_ptr<Configuration> loggerConfigurationOf(const std::filesystem::path &baseDirectory,
  const std::string &configuredProcess,
  const std::string &loggerLevel,
  const std::vector<LoggerSinkConfiguration> &sinks,
  bool withPathEntry = true,
  bool createSubDirectories = false)
{
  std::vector<std::shared_ptr<Entry>> entries;
  if (withPathEntry) {
    entries.push_back(std::make_shared<LoggerEntry>(
      type_name<LoggerComponent>(), std::make_shared<LoggerPathConfiguration>(baseDirectory, createSubDirectories)));
  }
  entries.push_back(std::make_shared<LoggerEntry>(type_name<LoggerComponent>(),
    std::make_shared<LoggerConfiguration>(configuredProcess, "", loggerLevel, false, sinks)));
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/tmp/nonexistent_logger_service_cfg");
  envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
  auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
  configuration->setEntries(entries);
  return configuration;
}

std::string loggerFileContent(const std::filesystem::path &file)
{
  std::ifstream stream(file);
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

bool loggerContains(const std::string &haystack, const std::string &needle)
{
  return haystack.find(needle) != std::string::npos;
}

LoggerSinkConfiguration loggerRotatingSink(const std::string &fileName,
  const std::string &level = "trace",
  std::size_t maxFileSize = 1024 * 1024,
  std::size_t maxFiles = 2)
{
  return LoggerSinkConfiguration(LoggerSinkConfiguration::RotatingFileSink, level, "", fileName, maxFileSize, maxFiles);
}

}// namespace

TEST_CASE("LoggerService: a configuration without a logger path throws", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", {}, false);
  auto processName = std::make_shared<ProcessName>("main");

  REQUIRE_THROWS_WITH(LoggerService(processName, configuration), "Please configure logger path");
}

TEST_CASE("LoggerService: a logger configuration of another process throws", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("worker");

  REQUIRE_THROWS_WITH(LoggerService(processName, configuration), "No matching Logger Configuration found");
}

TEST_CASE("LoggerService: the process name is matched as a regular expression", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main.*", "trace", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main_server");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "regex matched {}", 1);
  }

  REQUIRE(loggerContains(loggerFileContent(file), "regex matched 1"));
}

TEST_CASE("LoggerService: an empty sink file name falls back to <process name>.log", "[logger_service]")
{
  // resolveFilePath() derives the file name from the process when the configuration has none
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "main.log";

  REQUIRE_FALSE(std::filesystem::exists(file));
  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "fallback name");
  }

  REQUIRE(std::filesystem::exists(file));
  REQUIRE(loggerContains(loggerFileContent(file), "fallback name"));
  REQUIRE_FALSE(std::filesystem::exists(directory.path / ".log"));
}

TEST_CASE("LoggerService: a configured sink file name is used", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("custom") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "custom.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "named file");
  }

  REQUIRE(std::filesystem::exists(file));
  REQUIRE_FALSE(std::filesystem::exists(directory.path / "main.log"));
  REQUIRE(loggerContains(loggerFileContent(file), "named file"));
}

TEST_CASE("LoggerService: create sub directories puts the log into a per process directory", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration =
    loggerConfigurationOf(directory.path, "worker", "trace", { loggerRotatingSink("custom") }, true, true);
  auto processName = std::make_shared<ProcessName>("worker");
  const auto file = directory.path / "worker" / "custom.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "in sub directory");
  }

  REQUIRE(std::filesystem::exists(file));
  REQUIRE(loggerContains(loggerFileContent(file), "in sub directory"));
}

TEST_CASE("LoggerService: a log path that is a file throws", "[logger_service]")
{
  LoggerServiceLogDir directory;
  const auto blocked = directory.path / "not_a_directory";
  std::ofstream(blocked) << "x";
  auto configuration = loggerConfigurationOf(blocked, "main", "trace", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");

  REQUIRE_THROWS_WITH(LoggerService(processName, configuration), "path is file and not directory");
}

TEST_CASE("LoggerService: a daily sink with a malformed rotation time throws", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::DailyFileSink, "trace", "", "daily", "9:99") });
  auto processName = std::make_shared<ProcessName>("main");

  REQUIRE_THROWS_WITH(LoggerService(processName, configuration), "wrong configuration of time");
}

TEST_CASE("LoggerService: a daily sink creates a dated log file", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::DailyFileSink, "trace", "", "daily", "00:00") });
  auto processName = std::make_shared<ProcessName>("main");

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "daily entry");
  }

  // a daily sink names its file after the current date, so only the shape of the name is stable
  std::vector<std::filesystem::path> files;
  for (const auto &entry : std::filesystem::directory_iterator(directory.path)) { files.push_back(entry.path()); }
  REQUIRE(files.size() == 1);
  REQUIRE(StringUtils::startsWith(files[0].filename().string(), "daily_"));
  REQUIRE(files[0].extension() == ".log");
  REQUIRE(loggerContains(loggerFileContent(files[0]), "daily entry"));
}

TEST_CASE("LoggerService: messages below the logger level are dropped", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "warn", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::debug, "debug marker");
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "info marker");
    service.log(__FILE__, __LINE__, "test", LogLevel::error, "error marker");
  }

  const std::string content = loggerFileContent(file);
  REQUIRE_FALSE(loggerContains(content, "debug marker"));
  REQUIRE_FALSE(loggerContains(content, "info marker"));
  REQUIRE(loggerContains(content, "error marker"));
}

TEST_CASE("LoggerService: messages below the sink level are dropped", "[logger_service]")
{
  // the logger itself is at trace, only the sink filters - a separate switch in setSinkLevelPattern
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app", "err") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::warn, "warn marker");
    service.log(__FILE__, __LINE__, "test", LogLevel::error, "error marker");
  }

  const std::string content = loggerFileContent(file);
  REQUIRE_FALSE(loggerContains(content, "warn marker"));
  REQUIRE(loggerContains(content, "error marker"));
}

TEST_CASE("LoggerService: a configured sink pattern replaces the default pattern", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::RotatingFileSink, "trace", "%v", "app", 1024, 2) });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::error, "only the message");
  }

  const std::string content = loggerFileContent(file);
  REQUIRE(content == "only the message\n");
}

TEST_CASE("LoggerService: a broken format string is reported instead of throwing", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    REQUIRE_NOTHROW(service.log(__FILE__, __LINE__, "test", LogLevel::error, "missing {}"));
  }

  const std::string content = loggerFileContent(file);
  REQUIRE(loggerContains(content, "log formatting failed"));
  REQUIRE(loggerContains(content, "LoggerServiceTest.cpp"));
}

TEST_CASE("LoggerService: a syslog sink is skipped next to a file sink", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::SysLogSink, "trace", "", "my-syslog"),
      loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "next to syslog");
  }

  REQUIRE(loggerContains(loggerFileContent(file), "next to syslog"));
}

TEST_CASE("LoggerService: a console sink next to a file sink keeps the file sink", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::ConsoleSink, "trace", ""), loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "with console");
  }

  REQUIRE(loggerContains(loggerFileContent(file), "with console"));
}

TEST_CASE("LoggerService: the rotating sink rolls over when the size limit is exceeded", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration =
    loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app", "trace", 256, 2) });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";
  const auto firstRotation = directory.path / "app.1.log";
  const auto secondRotation = directory.path / "app.2.log";
  const auto thirdRotation = directory.path / "app.3.log";

  {
    LoggerService service(processName, configuration);
    for (int i = 0; i < 40; i++) { service.log(__FILE__, __LINE__, "test", LogLevel::info, "marker {}", i); }
  }

  REQUIRE(std::filesystem::exists(file));
  REQUIRE(std::filesystem::exists(firstRotation));
  // maxFiles = 2 keeps the indexes 1 and 2, everything older is dropped
  REQUIRE(std::filesystem::exists(secondRotation));
  REQUIRE_FALSE(std::filesystem::exists(thirdRotation));
  REQUIRE(loggerContains(loggerFileContent(firstRotation), "marker"));
  REQUIRE_FALSE(loggerContains(loggerFileContent(firstRotation), "marker 39"));
  REQUIRE(loggerContains(loggerFileContent(file), "marker 39"));
}

TEST_CASE("LoggerService: LOG_INFO dispatches through the configured singleton", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");
  const auto file = directory.path / "app.log";

  auto &service = LoggerService::getOrCreate(processName, configuration);
  LOG_INFO("singleton {}", 42);

  REQUIRE(&LoggerService::get() == &service);
  REQUIRE(loggerContains(loggerFileContent(file), "singleton 42"));
}

TEST_CASE("LoggerService: get() returns the same instance until it is replaced", "[logger_service]")
{
  LoggerService &first = LoggerService::get();

  REQUIRE(&LoggerService::get() == &first);
}
