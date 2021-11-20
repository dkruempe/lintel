#include <filesystem>
#include <memory>
#define CATCH_CONFIG_MAIN
#include <base_library/config.h>
#include <base_library/core/services/LoggerService.h>
#include <base_library/core/services/ProcessService.h>

#include <catch2/catch.hpp>
TEST_CASE("ProcessService Start Process Group") {
  Process df("df", {"-h"});
  Process ls("ls", {"-lrth"});
  bool startedDf = false;
  bool startedLs = false;
  int32_t stoppedDf = 0;
  int32_t stoppedLs = 0;
  auto onStartDf = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { startedDf = true; });
  auto onStartLs = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { startedLs = true; });
  df.addOnStartEvent(onStartDf);
  ls.addOnStartEvent(onStartLs);
  ProcessGroup group("testGroup", {df, ls});
  std::shared_ptr<ProcessName> processName =
      std::make_shared<ProcessName>("test");
  std::shared_ptr<EnvironmentConfiguration> env =
      std::make_shared<EnvironmentConfiguration>();
  ProcessService processService(processName, env);
  processService.startOf(group);
  REQUIRE(startedDf);
  REQUIRE(startedLs);
  processService.onShutdown();
}
TEST_CASE("ProcessService Start Process") {
  LOG_INFO("ProcessService Start Process Start");
  Process process("df", {"-h"});
  bool started = false;
  auto func = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { started = true; });
  process.addOnStartEvent(func);
  std::shared_ptr<ProcessName> processName =
      std::make_shared<ProcessName>("test");
  std::shared_ptr<EnvironmentConfiguration> env =
      std::make_shared<EnvironmentConfiguration>();
  ProcessService processService(processName, env);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  REQUIRE(future.get() == 0);
  processService.onShutdown();
  LOG_INFO("ProcessService Start Process End");
}
TEST_CASE("ProcessService Start/Restart Process") {
  LOG_INFO("ProcessServicie Start/Restart Process Start");
  Process process("ls", {"-la", "/"});
  process.enableAutoStart(5);
  bool started = false;
  int32_t restarted = 0;
  int32_t stopped = 0;
  auto onStart = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { started = true; });
  auto onRestart = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { restarted++; });
  auto onStop = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { stopped++; });
  process.addOnStartEvent(onStart);
  process.addOnRestartEvent(onRestart);
  process.addOnStopEvent(onStop);
  std::shared_ptr<ProcessName> processName =
      std::make_shared<ProcessName>("test");
  std::shared_ptr<EnvironmentConfiguration> env =
      std::make_shared<EnvironmentConfiguration>();
  ProcessService processService(processName, env);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  REQUIRE(future.get() == 0);
  REQUIRE(restarted == 5);
  REQUIRE(stopped == 1);
  processService.onShutdown();
  LOG_INFO("ProcessService Start/Restart Process End");
}
TEST_CASE("ProcessService Start/Stop Process") {
  Process process(std::string(PROJECT_PATH) +
                      std::filesystem::path::preferred_separator + ".." +
                      std::filesystem::path::preferred_separator +
                      "tests/services/test.sh",
                  {});
  bool started = false;
  int32_t stopped = 0;
  bool restarted = false;
  auto onStart = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { started = true; });
  auto onStop = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { stopped++; });
  auto onRestart = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { restarted = true; });
  process.addOnStartEvent(onStart);
  process.addOnStopEvent(onStop);
  process.addOnRestartEvent(onRestart);
  std::shared_ptr<ProcessName> processName =
      std::make_shared<ProcessName>("test");
  std::shared_ptr<EnvironmentConfiguration> env =
      std::make_shared<EnvironmentConfiguration>();
  ProcessService processService(processName, env);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  processService.restartOf(process);
  REQUIRE(restarted == true);
  processService.stopOf(process);
  REQUIRE(future.get() == 2);
  REQUIRE(stopped == 2);
  processService.onShutdown();
}