#include <catch2/catch_all.hpp>

#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/DatabaseConnectionComponent.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/property/configuration/PropertyComponent.h>
#include <lintel/features/property/configuration/PropertyRepositoryComponent.h>
#include <lintel/features/property/models/Property.h>
#include <lintel/features/property/models/PropertyRepositoryType.h>
#include <lintel/features/property/repositories/DatabasePropertyRepository.h>


#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

// `dpr` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `configuration` or `repository`
// would collide (and shadow local variables, Clang -Wshadow).
std::string dprDatabasePath()
{
  static int counter = 0;
  return (
    std::filesystem::temp_directory_path()
    / ("database_property_repository_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + ".db"))
    .string();
}

/**
 * Real SQLite property repository.
 *
 * The fixture can be broken on purpose (breakConnection()): the database path is
 * then occupied by a directory, so sqlite3_open fails and every repository method
 * hits the db::SQLException it is documented to swallow.
 */
struct DatabasePropertyRepositoryFixture
{
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_dpr_test_cfg" };
  std::string m_dbPath;
  std::shared_ptr<EnvironmentConfiguration> m_envConfig;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<DatabasePropertyRepository> m_repository;
  bool m_broken = false;

  DatabasePropertyRepositoryFixture()
  {
    m_dbPath = dprDatabasePath();
    std::remove(m_dbPath.c_str());
    std::error_code removeError;
    std::filesystem::remove_all(m_dbPath, removeError);
    createPropertyTable(m_dbPath);

    m_envConfig = std::make_shared<EnvironmentConfiguration>();
    m_configuration =
      std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{ std::make_shared<PropertyComponent>(),
                                        std::make_shared<PropertyRepositoryComponent>() },
        m_envConfig);
    auto connectionEntry = std::make_shared<DatabaseConnectionEntry>(
      type_name<DatabaseConnectionComponent>(), m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
    m_configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ connectionEntry });
    m_connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(m_configuration);
    m_processName = std::make_shared<ProcessName>("dprProcess");
    m_repository =
      std::make_shared<DatabasePropertyRepository>(m_connectionConfigurations, m_configuration, m_processName);
  }

  ~DatabasePropertyRepositoryFixture()
  {
    std::remove(m_dbPath.c_str());
    std::error_code error;
    std::filesystem::remove_all(m_dbPath, error);
  }

  DatabasePropertyRepositoryFixture(const DatabasePropertyRepositoryFixture &) = delete;
  DatabasePropertyRepositoryFixture &operator=(const DatabasePropertyRepositoryFixture &) = delete;

  /** Create the property table of cfg/database/DEFAULT_SQLITE/init_schema_default_version_1.sql */
  static void createPropertyTable(const std::string &path)
  {
    db::Connection connection(db::ConnectionType::SQLite, path);
    db::Statement statement(connection);
    statement.execute(R"(
            CREATE TABLE property (
                name          TEXT NOT NULL,
                instance_name TEXT NOT NULL,
                class_name    TEXT NOT NULL,
                process_name  TEXT NOT NULL,
                "type"        TEXT NOT NULL,
                value         TEXT NOT NULL,
                CONSTRAINT property_PK PRIMARY KEY (name, instance_name, class_name, process_name)
            );
        )");
  }

  /** Make every following database operation fail with a db::SQLException */
  void breakConnection()
  {
    std::remove(m_dbPath.c_str());
    std::error_code error;
    std::filesystem::remove_all(m_dbPath, error);
    std::filesystem::create_directories(m_dbPath, error);
    REQUIRE(!error);
    m_broken = true;
  }

  bool isBroken() const { return m_broken; }

  /** @return a property of this process, created in memory only */
  std::shared_ptr<PropertyBase> makeProperty(const std::string &name,
    const std::string &instance,
    const std::string &className,
    const std::string &processName,
    int32_t value) const
  {
    return std::make_shared<Property<int32_t>>(name, instance, className, processName, value, "", true);
  }
};

}// namespace

// --------------------------------------------------------------------------------------
// the normal round trip
// --------------------------------------------------------------------------------------

TEST_CASE("DatabasePropertyRepository::save followed by awake returns the stored properties",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  auto property = fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 4711);
  REQUIRE_NOTHROW(fixture.m_repository->save(property));

  const auto stored = fixture.m_repository->awake();
  REQUIRE(stored.size() == 1);
  REQUIRE(stored[0]->getName() == "dprInt");
  REQUIRE(stored[0]->getInstanceName() == "__DEFAULT");
  REQUIRE(stored[0]->getClassName() == "DprService");
  REQUIRE(stored[0]->getProcessName() == "dprProcess");
  REQUIRE(stored[0]->toString() == "4711");
  REQUIRE(stored[0]->getType() == "int32_t");
  REQUIRE(stored[0]->getDataStorage().getType() == PropertyRepositoryType::DATABASE_REPOSITORY);
}

TEST_CASE("DatabasePropertyRepository::save overwrites an existing row instead of duplicating it",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  fixture.m_repository->save(fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 1));
  fixture.m_repository->save(fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 2));

  const auto stored = fixture.m_repository->awake();
  REQUIRE(stored.size() == 1);
  REQUIRE(stored[0]->toString() == "2");
}

TEST_CASE("DatabasePropertyRepository::awake returns nothing for a different process", "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  fixture.m_repository->save(fixture.makeProperty("dprForeign", "__DEFAULT", "DprService", "otherProcess", 9));
  REQUIRE(fixture.m_repository->awake().empty());
}

TEST_CASE("DatabasePropertyRepository::allOf filters by process, class, instance and name",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  fixture.m_repository->save(std::vector<std::shared_ptr<PropertyBase>>{
    fixture.makeProperty("dprAlpha", "__DEFAULT", "DprService", "dprProcess", 1),
    fixture.makeProperty("dprBeta", "second", "DprService", "dprProcess", 2),
    fixture.makeProperty("dprGamma", "__DEFAULT", "OtherService", "dprProcess", 3),
    fixture.makeProperty("dprDelta", "__DEFAULT", "DprService", "otherProcess", 4) });

  // The default arguments of DatabasePropertyRepository::allOf() cannot be used: the header
  // declares instanceName = ".*0" instead of ".*"
  // (src/include/lintel/features/property/repositories/DatabasePropertyRepository.h:51), which becomes
  // "LIKE '%0'" and therefore matches nothing. The filters are passed explicitly here and the defect
  // is reported instead of being locked into the test.
  REQUIRE(fixture.m_repository->allOf(".*", ".*", ".*", ".*").size() == 4);
  REQUIRE(fixture.m_repository->allOf("dprProcess", ".*", ".*", ".*").size() == 3);
  REQUIRE(fixture.m_repository->allOf("dprProcess", "DprService", ".*", ".*").size() == 2);
  REQUIRE(fixture.m_repository->allOf("dprProcess", "DprService", "second").size() == 1);
  REQUIRE(fixture.m_repository->allOf("dprProcess", "DprService", ".*", "dprAlpha").size() == 1);
  REQUIRE(fixture.m_repository->allOf("dprProcess", "DprService", ".*", "dprA.*").size() == 1);
  REQUIRE(fixture.m_repository->allOf("nobody", ".*", ".*", ".*").empty());

  const auto alpha = fixture.m_repository->allOf("dprProcess", "DprService", ".*", "dprAlpha");
  REQUIRE(alpha.size() == 1);
  REQUIRE(alpha[0]->toString() == "1");
}

TEST_CASE("DatabasePropertyRepository::deleteOf removes the given properties", "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  auto alpha = fixture.makeProperty("dprAlpha", "__DEFAULT", "DprService", "dprProcess", 1);
  auto beta = fixture.makeProperty("dprBeta", "__DEFAULT", "DprService", "dprProcess", 2);
  auto gamma = fixture.makeProperty("dprGamma", "__DEFAULT", "DprService", "dprProcess", 3);
  fixture.m_repository->save(std::vector<std::shared_ptr<PropertyBase>>{ alpha, beta, gamma });
  REQUIRE(fixture.m_repository->awake().size() == 3);

  fixture.m_repository->deleteOf({ alpha, gamma });

  const auto remaining = fixture.m_repository->awake();
  REQUIRE(remaining.size() == 1);
  REQUIRE(remaining[0]->getName() == "dprBeta");
}

TEST_CASE("DatabasePropertyRepository reports its type and storage", "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;

  REQUIRE(fixture.m_repository->getType() == PropertyRepositoryType::DATABASE_REPOSITORY);
  // no entry of that type in the fixture configuration => the repository stays disabled and
  // therefore not mutable, which is what PropertyService::filterMutableRepositories() looks at
  REQUIRE_FALSE(fixture.m_repository->isEnabled());
  REQUIRE_FALSE(fixture.m_repository->isMutable());
  REQUIRE(fixture.m_repository->getDataStorage().getType() == PropertyRepositoryType::DATABASE_REPOSITORY);
}

// --------------------------------------------------------------------------------------
// the deliberate "best effort" behaviour against a broken connection
// --------------------------------------------------------------------------------------

TEST_CASE("DatabasePropertyRepository::save swallows a db::SQLException of a broken connection",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;
  fixture.breakConnection();
  REQUIRE(fixture.isBroken());

  auto property = fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 4711);

  // PropertyService::getOrCreate() calls save() without an error channel; a propagated exception
  // would abort the process startup as soon as the database is temporarily unreachable
  REQUIRE_NOTHROW(fixture.m_repository->save(property));
  REQUIRE_NOTHROW(fixture.m_repository->save(std::vector<std::shared_ptr<PropertyBase>>{ property }));
}

TEST_CASE("DatabasePropertyRepository::awake returns an empty result for a broken connection",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;
  fixture.breakConnection();

  std::vector<std::shared_ptr<PropertyBase>> awakeResult;
  REQUIRE_NOTHROW(awakeResult = fixture.m_repository->awake());
  // an empty result means "keep the configured default", not "data lost"
  REQUIRE(awakeResult.empty());
}

TEST_CASE("DatabasePropertyRepository::allOf returns an empty result for a broken connection",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;
  fixture.breakConnection();

  std::vector<std::shared_ptr<PropertyBase>> allOfResult;
  REQUIRE_NOTHROW(allOfResult = fixture.m_repository->allOf());
  REQUIRE(allOfResult.empty());
  REQUIRE(fixture.m_repository->allOf("dprProcess", "DprService", ".*", "dpr.*").empty());
}

TEST_CASE("DatabasePropertyRepository::deleteOf swallows a db::SQLException of a broken connection",
  "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;
  fixture.breakConnection();

  auto property = fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 4711);
  REQUIRE_NOTHROW(fixture.m_repository->deleteOf({ property }));
}

TEST_CASE("DatabasePropertyRepository recovers as soon as the connection works again", "[database_property_repository]")
{
  DatabasePropertyRepositoryFixture fixture;
  fixture.breakConnection();
  REQUIRE(fixture.m_repository->awake().empty());

  // the failed calls must not have poisoned the repository itself
  std::error_code error;
  std::filesystem::remove_all(fixture.m_dbPath, error);
  DatabasePropertyRepositoryFixture::createPropertyTable(fixture.m_dbPath);

  fixture.m_repository->save(fixture.makeProperty("dprInt", "__DEFAULT", "DprService", "dprProcess", 5));
  const auto stored = fixture.m_repository->awake();
  REQUIRE(stored.size() == 1);
  REQUIRE(stored[0]->toString() == "5");
}
