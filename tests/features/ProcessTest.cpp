#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include "lintel/features/base/models/Process.h"

namespace lintel {
namespace {

  std::shared_ptr<std::function<void(const Process &)>> procTestCounter(int &calls)
  {
    return std::make_shared<std::function<void(const Process &)>>([&calls](const Process &) { calls++; });
  }

}// namespace

TEST_CASE("Process exposes its definition")
{
  Process process(std::filesystem::path("/usr/bin/sleep"), { "10", "--verbose" }, "worker");
  REQUIRE(process.getPath() == std::filesystem::path("/usr/bin/sleep"));
  REQUIRE(process.getArgs() == std::vector<std::string>{ "10", "--verbose" });
  REQUIRE(process.getConfigName() == "worker");
  REQUIRE_FALSE(process.getId().empty());
}

TEST_CASE("Process callbacks fire only when registered")
{
  Process process("/bin/true", {}, {});

  // no callbacks registered yet -> early returns
  REQUIRE_NOTHROW(process.onStart());
  REQUIRE_NOTHROW(process.onStop());
  REQUIRE_NOTHROW(process.onRestart());
  REQUIRE_NOTHROW(process.onTerminate());
  REQUIRE_NOTHROW(process.onFinish());

  int starts = 0;
  int stops = 0;
  int restarts = 0;
  int terminates = 0;
  int finishes = 0;
  process.addOnStartEvent(procTestCounter(starts));
  process.addOnStopEvent(procTestCounter(stops));
  process.addOnRestartEvent(procTestCounter(restarts));
  process.addOnTerminateEvent(procTestCounter(terminates));
  process.addOnFinishEvent(procTestCounter(finishes));

  process.onStart();
  process.onStop();
  process.onRestart();
  process.onTerminate();
  process.onFinish();

  REQUIRE(starts == 1);
  REQUIRE(stops == 1);
  REQUIRE(restarts == 1);
  REQUIRE(terminates == 1);
  REQUIRE(finishes == 1);
}

TEST_CASE("Process auto-restart can be enabled and disabled")
{
  Process process("/bin/true", {}, {});
  REQUIRE_FALSE(process.isAutoRestart());

  process.enableAutoStart(5);
  REQUIRE(process.isAutoRestart());
  REQUIRE(process.getMaxAutoRestarts() == 5);

  process.enableAutoStart();// default -1 == unlimited
  REQUIRE(process.getMaxAutoRestarts() == -1);

  process.disableAutoStart();
  REQUIRE_FALSE(process.isAutoRestart());
  REQUIRE(process.getMaxAutoRestarts() == 0);
}

TEST_CASE("Process tracks restart counters")
{
  Process process("/bin/true", {}, {});
  REQUIRE(process.currentRestarts() == 0);
  process.increaseRestarts();
  process.increaseRestarts();
  REQUIRE(process.currentRestarts() == 2);
  process.resetRestarts();
  REQUIRE(process.currentRestarts() == 0);
}

TEST_CASE("Process round-trips its automation policy")
{
  Process process("/bin/true", {}, {});

  process.setRestartDelay(std::chrono::milliseconds(100));
  process.setRestartDelayMax(std::chrono::milliseconds(1000));
  process.setMinUptime(std::chrono::milliseconds(500));
  process.setMaxRestartRate(4);
  process.setRestartWindow(std::chrono::milliseconds(60000));
  process.setRestartInterval(std::chrono::milliseconds(3600000));
  process.setCpuNotify(75.5);
  process.setMemoryNotify(1024u * 1024u);

  REQUIRE(process.getRestartDelay() == std::chrono::milliseconds(100));
  REQUIRE(process.getRestartDelayMax() == std::chrono::milliseconds(1000));
  REQUIRE(process.getMinUptime() == std::chrono::milliseconds(500));
  REQUIRE(process.getMaxRestartRate() == 4);
  REQUIRE(process.getRestartWindow() == std::chrono::milliseconds(60000));
  REQUIRE(process.getRestartInterval() == std::chrono::milliseconds(3600000));
  REQUIRE(process.getCpuNotify().value() == Catch::Approx(75.5));
  REQUIRE(process.getMemoryNotify().value() == 1024u * 1024u);
}

TEST_CASE("Process active window logic covers all branches")
{
  Process process("/bin/true", {}, {});

  // no window configured -> always active
  REQUIRE_FALSE(process.hasActiveWindow());
  REQUIRE(process.inActiveWindow());

  // full-day window -> active regardless of the current hour
  process.setActiveFromHour(0);
  process.setActiveToHour(24);
  REQUIRE(process.hasActiveWindow());
  REQUIRE(process.getActiveFromHour().value() == 0);
  REQUIRE(process.getActiveToHour().value() == 24);
  REQUIRE(process.inActiveWindow());

  // empty window -> never active
  process.setActiveToHour(0);
  REQUIRE_FALSE(process.inActiveWindow());

  // window wrapping over midnight exercises the wrap branch
  process.setActiveFromHour(22);
  process.setActiveToHour(6);
  REQUIRE_NOTHROW(process.inActiveWindow());
}

}// namespace lintel
