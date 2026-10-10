#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/base/services/SchedulerService.h>

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

/** Deadline used whenever a test waits for a scheduled task to complete. */
constexpr std::chrono::seconds kSchedSvcDeadline{ 5 };

/** Long enough to prove "this task must not run" without wasting test time. */
constexpr std::chrono::milliseconds kSchedSvcQuiet{ 150 };

/**
 * Owns a `SchedulerService`. `start()` has to be called explicitly by the tests that need the
 * worker threads; the destructor shuts the service down so no thread outlives the test case.
 */
class schedSvcFixture
{
public:
  schedSvcFixture()
    : m_processName(std::make_shared<ProcessName>("scheduler-service-test")),
      m_scheduler(std::make_shared<SchedulerService>(m_processName))
  {}

  ~schedSvcFixture() { m_scheduler->onShutdown(); }

  schedSvcFixture(const schedSvcFixture &) = delete;
  schedSvcFixture &operator=(const schedSvcFixture &) = delete;

  /** Start the worker threads (the shipped default is 40 per instance) */
  void start() { m_scheduler->onInitialize(); }

  [[nodiscard]] SchedulerService &scheduler() { return *m_scheduler; }

  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<SchedulerService> m_scheduler;
};

/** @return true if the value became reachable before the deadline */
template<typename T> bool schedSvcAwaited(const std::future<T> &future)
{
  return future.wait_for(kSchedSvcDeadline) == std::future_status::ready;
}

}// namespace

TEST_CASE("SchedulerService: schedule runs an immediate task and resolves its future")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<bool> schedSvcRan{ false };
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule([&schedSvcRan]() {
    schedSvcRan.store(true);
    return 42;
  });

  REQUIRE(schedSvcAwaited(schedSvcFuture));
  REQUIRE(schedSvcFuture.get() == 42);
  REQUIRE(schedSvcRan.load());
}

TEST_CASE("SchedulerService: schedule forwards its arguments to the task")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  auto schedSvcFuture = schedSvcFixture.scheduler().schedule(
    [](int schedSvcNumber, std::string schedSvcText) { return schedSvcText + std::to_string(schedSvcNumber); },
    7,
    std::string{ "n" });

  REQUIRE(schedSvcAwaited(schedSvcFuture));
  REQUIRE(schedSvcFuture.get() == "n7");
}

TEST_CASE("SchedulerService: several scheduled tasks all run")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::vector<std::future<int>> schedSvcFutures;
  for (int schedSvcIndex = 0; schedSvcIndex < 3; schedSvcIndex++) {
    schedSvcFutures.push_back(schedSvcFixture.scheduler().schedule([schedSvcIndex]() { return schedSvcIndex; }));
  }

  int schedSvcSum = 0;
  for (auto &schedSvcFuture : schedSvcFutures) {
    REQUIRE(schedSvcAwaited(schedSvcFuture));
    schedSvcSum += schedSvcFuture.get();
  }
  REQUIRE(schedSvcSum == 3);
}

TEST_CASE("SchedulerService: schedule_after does not run the task before the delay elapsed")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<int> schedSvcRuns{ 0 };
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule_after(std::chrono::milliseconds(200), [&schedSvcRuns]() {
    schedSvcRuns.fetch_add(1);
    return 1;
  });

  // the task is 200ms in the future, so it cannot possibly have run yet
  REQUIRE(schedSvcFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);
  REQUIRE(schedSvcRuns.load() == 0);

  REQUIRE(schedSvcAwaited(schedSvcFuture));
  REQUIRE(schedSvcFuture.get() == 1);
  REQUIRE(schedSvcRuns.load() == 1);
}

TEST_CASE("SchedulerService: schedule_at waits for the given point in time")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<bool> schedSvcRan{ false };
  const auto schedSvcDue = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule_at(schedSvcDue, [&schedSvcRan]() {
    schedSvcRan.store(true);
    return 0;
  });

  REQUIRE(schedSvcFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);

  REQUIRE(schedSvcAwaited(schedSvcFuture));
  REQUIRE(schedSvcRan.load());
}

TEST_CASE("SchedulerService: schedule_at with a point in the past runs the task")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<bool> schedSvcRan{ false };
  const auto schedSvcPast = std::chrono::steady_clock::now() - std::chrono::seconds(1);
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule_at(schedSvcPast, [&schedSvcRan]() {
    schedSvcRan.store(true);
    return 0;
  });

  REQUIRE(schedSvcAwaited(schedSvcFuture));
  REQUIRE(schedSvcRan.load());
}

TEST_CASE("SchedulerService: schedule_at_fixed_rate runs the task repeatedly and stops on shutdown")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<int> schedSvcRuns{ 0 };
  schedSvcFixture.scheduler().schedule_at_fixed_rate(
    std::chrono::milliseconds(0), std::chrono::milliseconds(40), [&schedSvcRuns]() { schedSvcRuns.fetch_add(1); });

  const auto schedSvcDeadline = std::chrono::steady_clock::now() + kSchedSvcDeadline;
  while (schedSvcRuns.load() < 3 && std::chrono::steady_clock::now() < schedSvcDeadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  const int schedSvcRunsBeforeShutdown = schedSvcRuns.load();
  REQUIRE(schedSvcRunsBeforeShutdown >= 3);

  schedSvcFixture.scheduler().onShutdown();

  // a task that was already in flight when the shutdown started may still finish, so let
  // the service settle before taking the reference value
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  const int schedSvcSettled = schedSvcRuns.load();
  std::this_thread::sleep_for(kSchedSvcQuiet);
  REQUIRE(schedSvcRuns.load() == schedSvcSettled);
}

TEST_CASE("SchedulerService: clear removes tasks that are still pending")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<bool> schedSvcRan{ false };
  // 30s out, so the worker cannot have picked it up before clear() runs
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule_after(std::chrono::seconds(30), [&schedSvcRan]() {
    schedSvcRan.store(true);
    return 0;
  });

  schedSvcFixture.scheduler().clear();

  // dropping the task destroys its packaged_task, so the future resolves as broken promise
  REQUIRE(schedSvcFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  REQUIRE_THROWS_AS(schedSvcFuture.get(), std::future_error);
  REQUIRE(schedSvcRan.load() == false);
}

TEST_CASE("SchedulerService: onShutdown cancels tasks that are still pending")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  std::atomic<bool> schedSvcRan{ false };
  auto schedSvcFuture = schedSvcFixture.scheduler().schedule_after(std::chrono::seconds(30), [&schedSvcRan]() {
    schedSvcRan.store(true);
    return 0;
  });

  schedSvcFixture.scheduler().onShutdown();

  // the shutdown drops the pending task, so it never runs and its future breaks
  REQUIRE(schedSvcFuture.wait_for(kSchedSvcQuiet) == std::future_status::ready);
  REQUIRE_THROWS_AS(schedSvcFuture.get(), std::future_error);
  REQUIRE(schedSvcRan.load() == false);
}

TEST_CASE("SchedulerService: a failing task does not stop the scheduler")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  auto schedSvcFailing =
    schedSvcFixture.scheduler().schedule([]() -> int { throw std::runtime_error("scheduled task boom"); });

  REQUIRE(schedSvcAwaited(schedSvcFailing));
  REQUIRE_THROWS_AS(schedSvcFailing.get(), std::runtime_error);

  std::atomic<bool> schedSvcRan{ false };
  auto schedSvcFollowing = schedSvcFixture.scheduler().schedule([&schedSvcRan]() {
    schedSvcRan.store(true);
    return 1;
  });

  REQUIRE(schedSvcAwaited(schedSvcFollowing));
  REQUIRE(schedSvcRan.load());
}

TEST_CASE("SchedulerService: a task that throws a non standard exception is contained")
{
  schedSvcFixture schedSvcFixture;
  schedSvcFixture.start();

  auto schedSvcFailing = schedSvcFixture.scheduler().schedule([]() -> int { throw 7; });
  REQUIRE(schedSvcAwaited(schedSvcFailing));
  REQUIRE_THROWS_AS(schedSvcFailing.get(), int);

  std::atomic<bool> schedSvcRan{ false };
  auto schedSvcFollowing = schedSvcFixture.scheduler().schedule([&schedSvcRan]() {
    schedSvcRan.store(true);
    return 1;
  });

  REQUIRE(schedSvcAwaited(schedSvcFollowing));
  REQUIRE(schedSvcRan.load());
}