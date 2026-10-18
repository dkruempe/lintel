#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/ConnectionPoolRegistry.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/ScopedConnection.h"
#include "lintel/core/utils/TypeName.h"

#include "lintel/core/persistence/Transaction.h"

#include <catch2/catch_all.hpp>

#include "../helpers/PlatformCompat.h"

namespace lintel {
namespace db {
  namespace {
    std::string scopedTestUnique(const std::string &tag)
    {
      static unsigned int counter = 0;
      return "scopedconn_" + tag + "_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
    }

    std::shared_ptr<DatabaseConnectionEntry> scopedTestEntry(const std::string &tag)
    {
      const auto path = std::filesystem::temp_directory_path() / (scopedTestUnique(tag) + ".db");
      return std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
        path.string(),
        "",
        "",
        db::ConnectionType::SQLite,
        scopedTestUnique(tag + "_name"),
        -1,
        "",
        true);
    }

  }// namespace

  TEST_CASE("ScopedConnection: uses direct connection when no pool is registered", "[scoped_connection]")
  {
    ConnectionPoolRegistry::instance().clear();
    auto entry = scopedTestEntry("nopool");
    {
      db::ScopedConnection conn(entry);
      REQUIRE(conn.valid());
      REQUIRE_FALSE(conn.isPooled());
      REQUIRE(conn.connection().isHealthy());
    }
    ConnectionPoolRegistry::instance().clear();
  }

  TEST_CASE("ScopedConnection: uses pooled lease when pool is registered", "[scoped_connection]")
  {
    ConnectionPoolRegistry::instance().clear();
    auto entry = scopedTestEntry("pooled");
    auto pool = std::make_shared<ConnectionPool>(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) });
    ConnectionPoolRegistry::instance().registerPool(entry->getName(), pool);
    {
      db::ScopedConnection conn(entry);
      REQUIRE(conn.valid());
      REQUIRE(conn.isPooled());
      REQUIRE(conn.connection().isHealthy());
      REQUIRE(pool->outstanding() == 1);
    }
    REQUIRE(pool->outstanding() == 0);
    REQUIRE(pool->available() == 1);
    ConnectionPoolRegistry::instance().clear();
  }

  TEST_CASE("ScopedConnection: transaction works over pooled path", "[scoped_connection]")
  {
    ConnectionPoolRegistry::instance().clear();
    auto entry = scopedTestEntry("txpooled");
    auto pool = std::make_shared<ConnectionPool>(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) });
    ConnectionPoolRegistry::instance().registerPool(entry->getName(), pool);
    {
      db::ScopedConnection conn(entry);
      db::Transaction tx(conn.connection());
      tx.commit();
    }
    REQUIRE(pool->outstanding() == 0);
    ConnectionPoolRegistry::instance().clear();
  }

  TEST_CASE("ScopedConnection: moved-from is empty", "[scoped_connection]")
  {
    ConnectionPoolRegistry::instance().clear();
    auto entry = scopedTestEntry("move");
    db::ScopedConnection conn(entry);
    db::ScopedConnection moved(std::move(conn));
    REQUIRE_FALSE(conn.valid());
    REQUIRE(moved.valid());
    REQUIRE_THROWS_AS(conn.connection(), SQLException);
    REQUIRE(moved.connection().isHealthy());
    ConnectionPoolRegistry::instance().clear();
  }

}// namespace db
}// namespace lintel
