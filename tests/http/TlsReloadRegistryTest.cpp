#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <catch2/catch_all.hpp>

#include "lintel/features/http/service/TlsReloadRegistry.h"

/**
 * TlsReloadRegistry collects the reloadable resources behind
 * `POST /admin/reload-tls`; the HTTP server registers the TLS certificate pair
 * of the running process here.
 *
 * Covered: the empty registry, registration order, replacement of an existing
 * name, unregisterHandler(), a handler that reports a reason, one that throws a
 * non standard exception, clear(), and a handler that registers another handler
 * while reload() runs (the registry must not hold its lock across a handler).
 * The HTTP side of the endpoint is covered by AdminControllerTest, the TLS
 * behaviour of the server by HttpIntegrationTest.
 */

namespace lintel {
namespace {

  /** Empties the process-wide registry around a test case.
   *
   * The registry is a singleton; ctest starts one process per test case, but a
   * direct run of base_tests does not, so every case clears it itself. */
  struct TrRegistryGuard
  {
    TrRegistryGuard() { TlsReloadRegistry::instance().clear(); }
    ~TrRegistryGuard() { TlsReloadRegistry::instance().clear(); }
  };

}// namespace

TEST_CASE("TlsReloadRegistry: instance returns one process-wide registry")
{
  REQUIRE(&TlsReloadRegistry::instance() == &TlsReloadRegistry::instance());
}

TEST_CASE("TlsReloadRegistry: an empty registry reloads nothing")
{
  TrRegistryGuard guard;
  REQUIRE(TlsReloadRegistry::instance().reload().empty());
}

TEST_CASE("TlsReloadRegistry: reload reports every handler in registration order")
{
  TrRegistryGuard guard;
  TlsReloadRegistry::instance().registerHandler("first", []() { return std::string(); });
  TlsReloadRegistry::instance().registerHandler("second", []() { return std::string(); });

  const std::vector<TlsReloadRegistry::ReloadResult> results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 2);
  REQUIRE(results[0].m_name == "first");
  REQUIRE(results[0].m_reloaded);
  REQUIRE(results[0].m_detail.empty());
  REQUIRE(results[1].m_name == "second");
  REQUIRE(results[1].m_reloaded);
}

TEST_CASE("TlsReloadRegistry: a failing handler reports its reason")
{
  TrRegistryGuard guard;
  TlsReloadRegistry::instance().registerHandler(
    "tls", []() { return std::string("cannot read the certificate file '/etc/lintel/tls/server.crt'"); });

  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_reloaded);
  REQUIRE(results[0].m_detail == "cannot read the certificate file '/etc/lintel/tls/server.crt'");
}

TEST_CASE("TlsReloadRegistry: a throwing handler counts as failed")
{
  TrRegistryGuard guard;
  TlsReloadRegistry::instance().registerHandler("tls", []() -> std::string { throw std::runtime_error("boom"); });

  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_reloaded);
  REQUIRE(results[0].m_detail == "boom");
}

TEST_CASE("TlsReloadRegistry: a non standard exception is reported as unknown")
{
  TrRegistryGuard guard;
  TlsReloadRegistry::instance().registerHandler("tls", []() -> std::string { throw 42; });

  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE_FALSE(results[0].m_reloaded);
  REQUIRE(results[0].m_detail == "unknown exception");
}

TEST_CASE("TlsReloadRegistry: registering the same name replaces the handler")
{
  TrRegistryGuard guard;
  int replaced = 0;
  TlsReloadRegistry::instance().registerHandler("tls", [&replaced]() {
    ++replaced;
    return std::string();
  });
  TlsReloadRegistry::instance().registerHandler("tls", [&replaced]() {
    ++replaced;
    return std::string("still the old one");
  });

  // the second registration wins, and the name is not duplicated
  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE(replaced == 1);
  REQUIRE_FALSE(results[0].m_reloaded);
  REQUIRE(results[0].m_detail == "still the old one");
}

TEST_CASE("TlsReloadRegistry: a replaced owner cannot unregister its successor")
{
  TrRegistryGuard guard;
  // the owner of a name registers again under the same name (a server handing
  // over to the next one); the earlier id must not remove the new handler
  const auto first = TlsReloadRegistry::instance().registerHandler("tls", []() { return std::string(); });
  TlsReloadRegistry::instance().registerHandler("tls", []() { return std::string(); });

  TlsReloadRegistry::instance().unregisterHandler(first);

  REQUIRE(TlsReloadRegistry::instance().reload().size() == 1);
}

TEST_CASE("TlsReloadRegistry: unregisterHandler removes a single handler")
{
  TrRegistryGuard guard;
  const auto tlsId = TlsReloadRegistry::instance().registerHandler("tls", []() { return std::string(); });
  TlsReloadRegistry::instance().registerHandler("demo", []() { return std::string(); });

  TlsReloadRegistry::instance().unregisterHandler(tlsId);

  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE(results[0].m_name == "demo");

  // removing an id that is gone is a no-op
  TlsReloadRegistry::instance().unregisterHandler(tlsId);
  REQUIRE(TlsReloadRegistry::instance().reload().size() == 1);
}

TEST_CASE("TlsReloadRegistry: clear removes every handler")
{
  TrRegistryGuard guard;
  TlsReloadRegistry::instance().registerHandler("tls", []() { return std::string(); });
  TlsReloadRegistry::instance().registerHandler("demo", []() { return std::string(); });

  TlsReloadRegistry::instance().clear();

  REQUIRE(TlsReloadRegistry::instance().reload().empty());
}

TEST_CASE("TlsReloadRegistry: a handler may register another handler while reload runs")
{
  TrRegistryGuard guard;
  // reload() must not hold the registry lock across a handler: the server is
  // built while the bootstrap plugins register theirs, and both happen at
  // startup. A lock held across the handler would deadlock here.
  TlsReloadRegistry::instance().registerHandler("tls", []() {
    TlsReloadRegistry::instance().registerHandler("added-later", []() { return std::string(); });
    return std::string();
  });

  const auto results = TlsReloadRegistry::instance().reload();
  REQUIRE(results.size() == 1);
  REQUIRE(results[0].m_reloaded);

  // the handler added during the run is visible afterwards
  const auto after = TlsReloadRegistry::instance().reload();
  REQUIRE(after.size() == 2);
}

TEST_CASE("TlsReloadRegistry: handlers run concurrently with registration")
{
  TrRegistryGuard guard;
  for (int index = 0; index < 20; ++index) {
    TlsReloadRegistry::instance().registerHandler("handler-" + std::to_string(index), []() { return std::string(); });
  }

  std::thread worker([]() { TlsReloadRegistry::instance().reload(); });
  for (int index = 20; index < 40; ++index) {
    TlsReloadRegistry::instance().registerHandler("handler-" + std::to_string(index), []() { return std::string(); });
  }
  worker.join();

  REQUIRE(TlsReloadRegistry::instance().reload().size() >= 20);
}
}// namespace lintel
