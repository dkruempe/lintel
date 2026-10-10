#include <lintel/features/base/services/ExecutorService.h>

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

/** Deadline used whenever a test waits for a submitted task to complete. */
constexpr std::chrono::seconds kExecSvcDeadline{ 5 };

/** @return true if the value became reachable before the deadline */
template<typename T> bool execSvcAwaited(const std::future<T> &future)
{
  return future.wait_for(kExecSvcDeadline) == std::future_status::ready;
}

}// namespace

TEST_CASE("ExecutorService: execute runs a task and resolves its future")
{
  ExecutorService execSvcExecutor;

  std::atomic<bool> execSvcRan{ false };
  auto execSvcFuture = execSvcExecutor.execute([&execSvcRan]() {
    execSvcRan.store(true);
    return 42;
  });

  REQUIRE(execSvcAwaited(execSvcFuture));
  REQUIRE(execSvcFuture.get() == 42);
  REQUIRE(execSvcRan.load());
}

TEST_CASE("ExecutorService: execute forwards its arguments to the task")
{
  ExecutorService execSvcExecutor;

  auto execSvcFuture = execSvcExecutor.execute(
    [](int execSvcLeft, std::string execSvcRight) { return execSvcRight + std::to_string(execSvcLeft); },
    7,
    std::string{ "n" });

  REQUIRE(execSvcAwaited(execSvcFuture));
  REQUIRE(execSvcFuture.get() == "n7");
}

TEST_CASE("ExecutorService: execute runs the task on a different thread than the caller")
{
  ExecutorService execSvcExecutor;
  const std::thread::id execSvcCaller = std::this_thread::get_id();

  auto execSvcFuture = execSvcExecutor.execute([]() { return std::this_thread::get_id(); });

  REQUIRE(execSvcAwaited(execSvcFuture));
  REQUIRE(execSvcFuture.get() != execSvcCaller);
}

TEST_CASE("ExecutorService: submitted tasks run in submission order")
{
  ExecutorService execSvcExecutor;

  std::mutex execSvcMutex;
  std::vector<int> execSvcOrder;
  std::vector<std::future<void>> execSvcFutures;
  for (int execSvcIndex = 0; execSvcIndex < 5; execSvcIndex++) {
    execSvcFutures.push_back(execSvcExecutor.execute([execSvcIndex, &execSvcMutex, &execSvcOrder]() {
      std::lock_guard<std::mutex> execSvcLock(execSvcMutex);
      execSvcOrder.push_back(execSvcIndex);
    }));
  }
  for (auto &execSvcFuture : execSvcFutures) {
    REQUIRE(execSvcFuture.wait_for(kExecSvcDeadline) == std::future_status::ready);
  }

  // the single worker pops the queue front first, so the order is strictly FIFO
  const bool execSvcIsFifo = execSvcOrder == std::vector<int>{ 0, 1, 2, 3, 4 };
  REQUIRE(execSvcIsFifo);
}

TEST_CASE("ExecutorService: every submitted task runs")
{
  ExecutorService execSvcExecutor;

  std::atomic<int> execSvcRuns{ 0 };
  std::vector<std::future<void>> execSvcFutures;
  for (int execSvcIndex = 0; execSvcIndex < 50; execSvcIndex++) {
    execSvcFutures.push_back(execSvcExecutor.execute([&execSvcRuns]() { execSvcRuns.fetch_add(1); }));
  }
  for (auto &execSvcFuture : execSvcFutures) {
    REQUIRE(execSvcFuture.wait_for(kExecSvcDeadline) == std::future_status::ready);
  }
  REQUIRE(execSvcRuns.load() == 50);
}

TEST_CASE("ExecutorService: a failing task does not stop the worker")
{
  ExecutorService execSvcExecutor;

  auto execSvcFailing = execSvcExecutor.execute([]() -> int { throw std::runtime_error("task boom"); });

  REQUIRE(execSvcAwaited(execSvcFailing));
  REQUIRE_THROWS_AS(execSvcFailing.get(), std::runtime_error);

  std::atomic<bool> execSvcRan{ false };
  auto execSvcFollowing = execSvcExecutor.execute([&execSvcRan]() {
    execSvcRan.store(true);
    return 1;
  });

  REQUIRE(execSvcAwaited(execSvcFollowing));
  REQUIRE(execSvcRan.load());
}

TEST_CASE("ExecutorService: a task may submit another task")
{
  ExecutorService execSvcExecutor;

  // the future of the promise has to be taken before the task is queued, the worker may run
  // the task before execute() returns
  std::promise<std::future<int>> execSvcPromise;
  std::future<std::future<int>> execSvcInnerFuture = execSvcPromise.get_future();

  auto execSvcOuter = execSvcExecutor.execute(
    [&execSvcExecutor, &execSvcPromise]() { execSvcPromise.set_value(execSvcExecutor.execute([]() { return 7; })); });

  REQUIRE(execSvcAwaited(execSvcOuter));
  REQUIRE(execSvcInnerFuture.wait_for(kExecSvcDeadline) == std::future_status::ready);
  std::future<int> execSvcInner = execSvcInnerFuture.get();
  REQUIRE(execSvcAwaited(execSvcInner));
  REQUIRE(execSvcInner.get() == 7);
}

TEST_CASE("ExecutorService: the destructor drains the tasks that are still queued")
{
  std::mutex execSvcMutex;
  std::vector<int> execSvcExecuted;
  std::vector<std::future<void>> execSvcFutures;
  std::atomic<int> execSvcRuns{ 0 };

  {
    ExecutorService execSvcExecutor;
    for (int execSvcIndex = 0; execSvcIndex < 20; execSvcIndex++) {
      execSvcFutures.push_back(execSvcExecutor.execute([execSvcIndex, &execSvcMutex, &execSvcExecuted, &execSvcRuns]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard<std::mutex> execSvcLock(execSvcMutex);
        execSvcExecuted.push_back(execSvcIndex);
        execSvcRuns.fetch_add(1);
      }));
    }
    // the executor is destroyed here while at most a few tasks can have finished
  }

  // run() returns only once the queue is empty, so the destructor waits for the backlog
  REQUIRE(execSvcRuns.load() == 20);
  const bool execSvcIsFifo =
    execSvcExecuted == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
  REQUIRE(execSvcIsFifo);
  for (auto &execSvcFuture : execSvcFutures) {
    REQUIRE(execSvcFuture.wait_for(kExecSvcDeadline) == std::future_status::ready);
  }
}

TEST_CASE("ExecutorService: a service without tasks shuts down cleanly")
{
  std::atomic<bool> execSvcDestroyed{ false };
  {
    ExecutorService execSvcExecutor;
    execSvcExecutor.execute([]() { return 1; });
  }
  execSvcDestroyed.store(true);

  REQUIRE(execSvcDestroyed.load());
}