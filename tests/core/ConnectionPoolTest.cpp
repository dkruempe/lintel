#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/exceptions/PoolTimeoutException.h"
#include "lintel/core/exceptions/SQLException.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/utils/TypeName.h"

#include <catch2/catch_all.hpp>

#include "../helpers/PlatformCompat.h"

namespace lintel {
namespace db {
  namespace {

    // Atomic: the connector lambda runs poolTestUnique() from several worker
    // threads in the parallel-load test (TSan flagged the plain counter).
    std::string poolTestUnique(const std::string &tag)
    {
      static std::atomic<unsigned int> counter{ 0 };
      return "connpool_" + tag + "_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
    }

    /** A real SQLite connection entry (the pool opens it, so tests need no mock). */
    std::shared_ptr<DatabaseConnectionEntry> poolTestEntry(const std::string &tag)
    {
      const auto path = std::filesystem::temp_directory_path() / (poolTestUnique(tag) + ".db");
      return std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
        path.string(),
        "",
        "",
        db::ConnectionType::SQLite,
        poolTestUnique(tag + "_name"),
        -1,
        "",
        true);
    }

    /** Factory that opens real SQLite connections and counts them.
     *
     * Every connection gets its own database file: the parallel load test must
     * measure the pool, not SQLite's file lock (which is exactly why SQLite
     * defaults to `poolSize=1`). The counter is atomic because the pool connects
     * concurrently. */
    struct ConnectCounter
    {
      std::atomic<int> created{ 0 };

      ConnectionPool::Connector connector()
      {
        return [this]() -> std::unique_ptr<Connection> {
          ++created;
          const auto path = std::filesystem::temp_directory_path() / (poolTestUnique("conn") + ".db");
          return std::make_unique<Connection>(db::ConnectionType::SQLite, path.string());
        };
      }
    };

  }// namespace

  TEST_CASE("ConnectionPool: reuses a released connection instead of reconnecting", "[connection_pool]")
  {
    auto entry = poolTestEntry("reuse");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) }, {}, counter.connector());

    {
      auto lease = pool.acquire();
      REQUIRE(lease.valid());
      REQUIRE(pool.outstanding() == 1);
    }
    REQUIRE(pool.outstanding() == 0);
    REQUIRE(pool.available() == 1);

    {
      auto lease = pool.acquire();
      REQUIRE(lease.valid());
    }

    // the second acquire reused the parked connection, it did not connect again
    REQUIRE(counter.created == 1);
  }

  TEST_CASE("ConnectionPool: a bounded pool times out and lists the outstanding lease", "[connection_pool]")
  {
    auto entry = poolTestEntry("timeout");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(50) }, {}, counter.connector());

    std::string message;
    std::thread holder([&pool]() {
      auto lease = pool.acquire();
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    });
    while (pool.outstanding() == 0) { std::this_thread::yield(); }

    try {
      auto lease = pool.acquire();
      (void)lease;
      FAIL("acquire() had to time out, the single slot was held by another thread");
    } catch (const PoolTimeoutException &exception) {
      message = exception.what();
    }
    holder.join();

    REQUIRE(message.find("maxConnections=1") != std::string::npos);
    REQUIRE(message.find("outstanding leases") != std::string::npos);
    REQUIRE(message.find("thread ") != std::string::npos);
    // only one connection was ever opened
    REQUIRE(counter.created == 1);
  }

  TEST_CASE("ConnectionPool: nested acquire of the same pool fails fast", "[connection_pool]")
  {
    auto entry = poolTestEntry("reentrancy");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(5000) }, {}, counter.connector());

    auto lease = pool.acquire();
    const auto start = std::chrono::steady_clock::now();
    std::string message;
    try {
      auto nested = pool.acquire();
      (void)nested;
      FAIL("a nested acquire() of the same pool must throw");
    } catch (const PoolTimeoutException &exception) {
      message = exception.what();
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE(message.find("re-entrancy") != std::string::npos);
    // fail fast, not after the configured 5 s timeout
    REQUIRE(elapsed < std::chrono::milliseconds(1000));
  }

  TEST_CASE("ConnectionPool: a lease returns its connection while an exception unwinds", "[connection_pool]")
  {
    auto entry = poolTestEntry("exception");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) }, {}, counter.connector());

    REQUIRE_THROWS_AS(
      [&pool]() {
        auto lease = pool.acquire();
        throw std::runtime_error("lease scope left by exception");
      }(),
      std::runtime_error);

    REQUIRE(pool.outstanding() == 0);
    REQUIRE(pool.available() == 1);
  }

  TEST_CASE("ConnectionPool: a broken connection is discarded on release", "[connection_pool]")
  {
    auto entry = poolTestEntry("broken");
    ConnectCounter counter;
    int healthCalls = 0;
    const ConnectionPool::HealthCheck unhealthy = [&healthCalls](const Connection &) {
      ++healthCalls;
      return false;
    };
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) }, unhealthy, counter.connector());

    {
      auto lease = pool.acquire();
      REQUIRE(lease.valid());
    }
    REQUIRE(healthCalls == 1);
    REQUIRE(pool.available() == 0);

    // dropping the broken connection freed the slot, so the next acquire connects again
    {
      auto lease = pool.acquire();
      REQUIRE(lease.valid());
    }
    REQUIRE(counter.created == 2);
  }

  TEST_CASE("ConnectionPool: a healthy connection stays parked", "[connection_pool]")
  {
    auto entry = poolTestEntry("healthy");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 2, std::chrono::milliseconds(1000) }, {}, counter.connector());

    {
      auto lease = pool.acquire();
      REQUIRE(lease.connection().isHealthy());
    }

    REQUIRE(pool.available() == 1);
    REQUIRE(counter.created == 1);
  }

  TEST_CASE("ConnectionPool: parallel load stays within the bound and loses no lease", "[connection_pool]")
  {
    auto entry = poolTestEntry("load");
    ConnectCounter counter;
    constexpr std::size_t maxConnections = 4;
    ConnectionPool pool(
      entry, PoolSettings{ maxConnections, std::chrono::milliseconds(10000) }, {}, counter.connector());

    constexpr int threadCount = 8;
    constexpr int iterations = 25;
    std::atomic<int> completed{ 0 };
    std::vector<std::thread> workers;
    workers.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i) {
      workers.emplace_back([&pool, &completed]() {
        for (int iteration = 0; iteration < iterations; ++iteration) {
          auto lease = pool.acquire();
          (void)lease;
          ++completed;
        }
      });
    }
    for (auto &worker : workers) { worker.join(); }

    REQUIRE(completed == threadCount * iterations);
    REQUIRE(pool.outstanding() == 0);
    // never more than the configured maximum connections were opened
    REQUIRE(counter.created <= static_cast<int>(maxConnections));
  }

  TEST_CASE("ConnectionPool: poolSize=0 disables pooling", "[connection_pool]")
  {
    auto entry = poolTestEntry("disabled");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 0, std::chrono::milliseconds(50) }, {}, counter.connector());

    REQUIRE_THROWS_AS(pool.acquire(), PoolTimeoutException);
    REQUIRE(counter.created == 0);
  }

  TEST_CASE("ConnectionPool: a moved-from lease is empty and connection() throws", "[connection_pool]")
  {
    auto entry = poolTestEntry("lease_move");
    ConnectCounter counter;
    ConnectionPool pool(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) }, {}, counter.connector());

    auto lease = pool.acquire();
    ConnectionPool::Lease moved(std::move(lease));
    REQUIRE_FALSE(lease.valid());
    REQUIRE(moved.valid());
    // Calling connection() must not throw for a moved-to lease; comparing the
    // address of the returned reference against nullptr is a tautology (Clang
    // rejects it under -Werror), so exercise the reference instead.
    REQUIRE(moved.connection().isHealthy());
    moved = ConnectionPool::Lease{};

    ConnectionPool::Lease empty;
    REQUIRE_FALSE(empty.valid());
    REQUIRE_THROWS_AS(empty.connection(), SQLException);
    REQUIRE(pool.outstanding() == 0);
  }

}// namespace db
}// namespace lintel
