#include <string>
#include <thread>
#include <vector>

#include <catch2/catch_all.hpp>

#include "lintel/core/services/ReadinessRegistry.h"

/**
 * ReadinessRegistry collects the named dependency checks behind `GET /ready`.
 *
 * Covered: registration order, replacement of an existing name, the empty
 * registry, a check that reports a reason, one that only returns false, one that
 * throws, clear() and concurrent registration while checks run. The HTTP side of
 * the endpoint (status code and JSON body) is covered by HttpIntegrationTest.
 */

namespace lintel {
namespace {

  /** Empties the process-wide registry around a test case.
   *
   * The registry is a singleton; ctest starts one process per test case, but a
   * direct run of base_tests does not, so every case clears it itself. */
  struct RrRegistryGuard
  {
    RrRegistryGuard() { ReadinessRegistry::instance().clear(); }
    ~RrRegistryGuard() { ReadinessRegistry::instance().clear(); }
  };

}// namespace

TEST_CASE("ReadinessRegistry: instance returns one process-wide registry")
{
  REQUIRE(&ReadinessRegistry::instance() == &ReadinessRegistry::instance());
}

TEST_CASE("ReadinessRegistry: an empty registry runs no check")
{
  RrRegistryGuard guard;
  REQUIRE(ReadinessRegistry::instance().run().empty());
}

TEST_CASE("ReadinessRegistry: run reports every check in registration order")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("first", [](std::string &) { return true; });
  ReadinessRegistry::instance().registerCheck("second", [](std::string &) { return true; });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 2);
  REQUIRE(results[0].m_name == "first");
  REQUIRE(results[0].m_ready);
  REQUIRE(results[0].m_detail.empty());
  REQUIRE(results[1].m_name == "second");
  REQUIRE(results[1].m_ready);
}

TEST_CASE("ReadinessRegistry: a failing check reports its reason")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("database", [](std::string &detail) {
    detail = "connection refused";
    return false;
  });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_ready);
  REQUIRE(results[0].m_detail == "connection refused");
}

TEST_CASE("ReadinessRegistry: a failing check without a reason gets a default detail")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("database", [](std::string &) { return false; });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_ready);
  REQUIRE(results[0].m_detail == "check reported not ready");
}

TEST_CASE("ReadinessRegistry: a throwing check counts as failed with the exception message")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck(
    "database", [](std::string &) -> bool { throw std::runtime_error("boom"); });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_ready);
  REQUIRE(results[0].m_detail == "boom");
}

TEST_CASE("ReadinessRegistry: a check that throws a non-std exception still counts as failed")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("database", [](std::string &) -> bool { throw 42; });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_ready);
  REQUIRE(results[0].m_detail == "unknown exception");
}

TEST_CASE("ReadinessRegistry: registering an existing name replaces the check")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("database", [](std::string &) { return false; });
  ReadinessRegistry::instance().registerCheck("database", [](std::string &) { return true; });

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE(results[0].m_name == "database");
  REQUIRE(results[0].m_ready);
}

TEST_CASE("ReadinessRegistry: clear removes every check")
{
  RrRegistryGuard guard;
  ReadinessRegistry::instance().registerCheck("database", [](std::string &) { return true; });
  ReadinessRegistry::instance().clear();
  REQUIRE(ReadinessRegistry::instance().run().empty());
}

TEST_CASE("ReadinessRegistry: registration and runs are safe across threads")
{
  RrRegistryGuard guard;
  constexpr int kThreads = 8;

  std::vector<std::thread> threads;
  threads.reserve(static_cast<std::size_t>(kThreads));
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([i]() {
      for (int j = 0; j < 50; ++j) {
        ReadinessRegistry::instance().registerCheck("check_" + std::to_string(i), [](std::string &) { return true; });
        static_cast<void>(ReadinessRegistry::instance().run());
      }
    });
  }
  for (auto &thread : threads) { thread.join(); }

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == static_cast<std::size_t>(kThreads));
}

}// namespace lintel
