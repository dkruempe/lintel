#define CATCH_CONFIG_MAIN
#include <base_library/core/services/ProcessService.h>

#include <catch2/catch.hpp>

TEST_CASE("ProcessService Start Process") {
  Process process("df", {"-h"});
  bool started = false;
  auto func = std::make_shared<std::function<void(const Process &)>>(
      [&](const Process &process) { started = true; });
  process.addOnStartEvent(func);
  std::shared_ptr<ProcessName> processName =
      std::make_shared<ProcessName>("test");
  ProcessService processService(processName);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  REQUIRE(future.get() == 0);
  processService.onShutdown();
}
TEST_CASE("ProcessService Start/Restart Process") {
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
  ProcessService processService(processName);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  REQUIRE(future.get() == 0);
  REQUIRE(restarted == 5);
  REQUIRE(stopped == 1);
  processService.onShutdown();
}
TEST_CASE("ProcessService Start/Stop Process") {
  Process process(
      "/Users/dkruempe/CLionProjects/cpp-base-library/tests/services/test.sh",
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
  ProcessService processService(processName);
  std::future<int> future = processService.startOf(process);
  REQUIRE(started);
  processService.stopOf(process);
  REQUIRE(future.get() == 2);
  REQUIRE(stopped == 1);
  processService.onShutdown();
}