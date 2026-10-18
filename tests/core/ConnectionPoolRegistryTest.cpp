#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/exceptions/PoolNotFoundException.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/ConnectionPoolRegistry.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/utils/TypeName.h"

#include <catch2/catch_all.hpp>

#include "../helpers/PlatformCompat.h"

namespace lintel {
namespace db {
  namespace {

    /** Clears the process-wide registry before and after a test case. */
    struct RegistryGuard
    {
      RegistryGuard() { ConnectionPoolRegistry::instance().clear(); }
      ~RegistryGuard() { ConnectionPoolRegistry::instance().clear(); }
      RegistryGuard(const RegistryGuard &) = delete;
      RegistryGuard &operator=(const RegistryGuard &) = delete;
    };

    std::shared_ptr<ConnectionPool> registryTestPool(const std::string &name)
    {
      const auto path = std::filesystem::temp_directory_path()
                        / ("connpool_registry_" + name + "_" + std::to_string(::getpid()) + ".db");
      auto entry = std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
        path.string(),
        "",
        "",
        db::ConnectionType::SQLite,
        name,
        -1,
        "",
        true);
      return std::make_shared<ConnectionPool>(entry, PoolSettings{ 1, std::chrono::milliseconds(1000) });
    }

  }// namespace

  TEST_CASE("ConnectionPoolRegistry: starts empty and rejects an unknown name", "[connection_pool_registry]")
  {
    RegistryGuard guard;

    REQUIRE(ConnectionPoolRegistry::instance().empty());
    REQUIRE(ConnectionPoolRegistry::instance().size() == 0);
    REQUIRE_FALSE(ConnectionPoolRegistry::instance().hasPool("missing"));
    REQUIRE_THROWS_AS(ConnectionPoolRegistry::instance().poolOf("missing"), PoolNotFoundException);
  }

  TEST_CASE("ConnectionPoolRegistry: registers, finds and clears pools", "[connection_pool_registry]")
  {
    RegistryGuard guard;
    auto &registry = ConnectionPoolRegistry::instance();

    auto pool = registryTestPool("alpha");
    registry.registerPool("alpha", pool);

    REQUIRE_FALSE(registry.empty());
    REQUIRE(registry.size() == 1);
    REQUIRE(registry.hasPool("alpha"));
    REQUIRE(registry.poolOf("alpha") == pool);
    REQUIRE(registry.names() == std::vector<std::string>{ "alpha" });

    registry.clear();
    REQUIRE(registry.empty());
    REQUIRE_THROWS_AS(registry.poolOf("alpha"), PoolNotFoundException);
  }

  TEST_CASE("ConnectionPoolRegistry: a second registration replaces the pool", "[connection_pool_registry]")
  {
    RegistryGuard guard;
    auto &registry = ConnectionPoolRegistry::instance();

    auto first = registryTestPool("replace");
    auto second = registryTestPool("replace");
    registry.registerPool("replace", first);
    registry.registerPool("replace", second);

    REQUIRE(registry.size() == 1);
    REQUIRE(registry.poolOf("replace") == second);
  }

  TEST_CASE("ConnectionPoolRegistry: names are returned in order", "[connection_pool_registry]")
  {
    RegistryGuard guard;
    auto &registry = ConnectionPoolRegistry::instance();

    registry.registerPool("zeta", registryTestPool("zeta"));
    registry.registerPool("alpha", registryTestPool("alpha"));
    registry.registerPool("mike", registryTestPool("mike"));

    REQUIRE(registry.size() == 3);
    REQUIRE(registry.names() == std::vector<std::string>{ "alpha", "mike", "zeta" });
  }

}// namespace db
}// namespace lintel
