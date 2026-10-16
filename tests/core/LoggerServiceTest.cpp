#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../helpers/PlatformCompat.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif// _WIN32

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/configuration/LoggerComponent.h"
#include "lintel/core/configuration/LoggerConfiguration.h"
#include "lintel/core/configuration/LoggerEntry.h"
#include "lintel/core/configuration/LoggerPathConfiguration.h"
#include "lintel/core/configuration/LoggerSinkConfiguration.h"
#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/StringUtils.h"
#include "lintel/core/utils/TypeName.h"

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

namespace lintel {
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
    return LoggerSinkConfiguration(
      LoggerSinkConfiguration::RotatingFileSink, level, "", fileName, maxFileSize, maxFiles);
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

TEST_CASE("LoggerService: every sink level string is mapped", "[logger_service]")
{
  LoggerServiceLogDir directory;
  const std::vector<LoggerSinkConfiguration> sinks = { loggerRotatingSink("critical_sink", "critical"),
    loggerRotatingSink("warn_sink", "warn"),
    loggerRotatingSink("err_sink", "err"),
    loggerRotatingSink("info_sink", "info"),
    loggerRotatingSink("debug_sink", "debug"),
    loggerRotatingSink("trace_sink", "trace"),
    loggerRotatingSink("off_sink", "off") };
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", sinks);
  auto processName = std::make_shared<ProcessName>("main");

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::error, "level marker");
  }

  // the trace sink is the most permissive one and must have received the message
  REQUIRE(loggerContains(loggerFileContent(directory.path / "trace_sink.log"), "level marker"));
  // the off sink must never receive the message (it may still have been created empty)
  const auto offSink = directory.path / "off_sink.log";
  if (std::filesystem::exists(offSink)) { REQUIRE_FALSE(loggerContains(loggerFileContent(offSink), "level marker")); }
}

TEST_CASE("LoggerService: every logger level string is mapped", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto processName = std::make_shared<ProcessName>("main");
  const std::vector<std::string> levels = { "critical", "warn", "err", "info", "debug", "trace", "off" };

  for (const auto &level : levels) {
    INFO("logger level: " << level);
    auto configuration = loggerConfigurationOf(directory.path, "main", level, { loggerRotatingSink("app") });
    LoggerService service(processName, configuration);
    REQUIRE_NOTHROW(service.log(__FILE__, __LINE__, "test", LogLevel::error, "level {}", level));
  }
}

#if !defined(_WIN32)
TEST_CASE("LoggerService: a tcp sink is constructed and used", "[logger_service]")
{
  // a listening socket lets the tcp sink connect during construction
  const int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
  REQUIRE(serverFd >= 0);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = ::inet_addr("127.0.0.1");
  address.sin_port = 0;
  REQUIRE(::bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
  REQUIRE(::listen(serverFd, 4) == 0);
  socklen_t length = sizeof(address);
  REQUIRE(::getsockname(serverFd, reinterpret_cast<sockaddr *>(&address), &length) == 0);
  // Unqualified: macOS defines ntohs as a macro (see tests/helpers/NetCompat.h).
  const int32_t port = ntohs(address.sin_port);

  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path,
    "main",
    "trace",
    { LoggerSinkConfiguration(LoggerSinkConfiguration::TcpSink, "trace", "", "127.0.0.1", port),
      loggerRotatingSink("app") });
  auto processName = std::make_shared<ProcessName>("main");

  {
    LoggerService service(processName, configuration);
    service.log(__FILE__, __LINE__, "test", LogLevel::info, "tcp marker");
  }

  ::close(serverFd);
  REQUIRE(loggerContains(loggerFileContent(directory.path / "app.log"), "tcp marker"));
}
#endif// !_WIN32

TEST_CASE("LoggerService: initSingleton installs a configured logger", "[logger_service]")
{
  LoggerServiceLogDir directory;
  auto configuration = loggerConfigurationOf(directory.path, "main", "trace", { loggerRotatingSink("app") });

  LoggerService::initSingleton(std::make_shared<ProcessName>("main"), configuration);
  REQUIRE_NOTHROW(LoggerService::get().log(__FILE__, __LINE__, "test", LogLevel::error, "singleton"));

  // initSingleton2 falls back to the default logger and must not throw
  REQUIRE_NOTHROW(LoggerService::initSingleton2());
  REQUIRE_NOTHROW(LoggerService::get());
}
}// namespace lintel
