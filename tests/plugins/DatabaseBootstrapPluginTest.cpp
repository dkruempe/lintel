#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "lintel/config.h"
#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/plugins/DatabaseBootstrapPlugin.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/DatabaseConnectionComponent.h"
#include "lintel/features/base/configuration/DatabaseConnectionEntry.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

#include <catch2/catch_all.hpp>

/**
 * DatabaseBootstrapPlugin applies the SQL files of <CONFIG_DIRECTORY>/database/<connection name>.
 *
 * The directory is resolved from the compile time macro CONFIG_DIRECTORY (see
 * DatabaseBootstrapPlugin.h), *not* from EnvironmentConfiguration, so a test that wants its own
 * SQL files has to create a directory below the real configuration tree - and the "environment is
 * ignored" behaviour is pinned by its own test case.
 */

namespace {

std::string dbpUnique(const std::string &tag)
{
  static unsigned int counter = 0;
  return "dbp_" + tag + "_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
}

std::filesystem::path dbpDatabaseRoot() { return std::filesystem::path(CONFIG_DIRECTORY) / "database"; }

/** Fresh, empty SQLite database file below the temporary directory. */
std::filesystem::path dbpFreshDatabase(const std::string &tag)
{
  const auto path = std::filesystem::temp_directory_path() / (dbpUnique(tag) + ".db");
  std::filesystem::remove(path);
  return path;
}

/** Create <CONFIG_DIRECTORY>/database/<name> and return it. */
std::filesystem::path dbpCreateSqlDirectory(const std::string &name)
{
  const auto path = dbpDatabaseRoot() / name;
  std::filesystem::remove_all(path);
  std::filesystem::create_directories(path);
  return path;
}

void dbpWriteFile(const std::filesystem::path &directory, const std::string &fileName, const std::string &content)
{
  std::ofstream file(directory / fileName);
  file << content;
}

/** @return the single scalar of a query, "" if the query returned no row */
std::string dbpScalar(const std::filesystem::path &databaseFile, const std::string &query)
{
  db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
  db::Statement statement(connection);
  db::Result result = statement.execute(query);
  if (result.getSize() == 0) { return ""; }
  return result.getValue(0, 0);
}

/** @return the number of rows the query returns */
int dbpCount(const std::filesystem::path &databaseFile, const std::string &query)
{
  const std::string value = dbpScalar(databaseFile, "select count(*) from (" + query + ")");
  return value.empty() ? -1 : std::stoi(value);
}

/** @return true if the table exists, i.e. the query does not fail */
bool dbpTableExists(const std::filesystem::path &databaseFile, const std::string &table)
{
  try {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    statement.execute("select * from " + table + " where 1 = 0");
    return true;
  } catch (const db::SQLException &) {
    return false;
  }
}

/** @return the recorded schema version, -1 if the schema is not recorded */
int dbpSchemaVersion(const std::filesystem::path &databaseFile, const std::string &schema)
{
  const std::string value = dbpScalar(databaseFile, "select version from schema_version where name = '" + schema + "'");
  return value.empty() ? -1 : std::stoi(value);
}

/**
 * Fixture with one SQLite connection entry.
 *
 * - shippedConfigName: use an existing directory of the configuration tree (e.g. "DEFAULT_SQLITE")
 *   and do not touch it. The plugin then works on files that are part of the repository.
 * - otherwise: create an empty, uniquely named directory below <CONFIG_DIRECTORY>/database and
 *   remove it again in the destructor.
 * - configDirectory: value handed to EnvironmentConfiguration, only relevant for the test that
 *   pins that the plugin ignores it.
 */
struct DbpFixture
{
  std::string connectionName;
  std::filesystem::path databaseFile;
  std::filesystem::path sqlDirectory;
  std::vector<std::shared_ptr<Entry>> entries;
  std::shared_ptr<Configuration> configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
  std::shared_ptr<DatabaseBootstrapPlugin> plugin;

  explicit DbpFixture(const std::string &tag,
    const std::string &shippedConfigName = {},
    const std::filesystem::path &configDirectory = {})
    : connectionName(shippedConfigName.empty() ? dbpUnique(tag) : shippedConfigName),
      databaseFile(dbpFreshDatabase(tag))
  {
    if (shippedConfigName.empty()) { sqlDirectory = dbpCreateSqlDirectory(connectionName); }
    std::filesystem::path configDir =
      configDirectory.empty() ? std::filesystem::path("/tmp/nonexistent_dbp_cfg") / dbpUnique(tag) : configDirectory;
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, configDir.string());
    envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, dbpUnique("bootstrap"));
    entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
      databaseFile.string(),
      "",
      "",
      db::ConnectionType::SQLite,
      connectionName,
      -1,
      "",
      true));
    build();
  }

  ~DbpFixture()
  {
    if (!sqlDirectory.empty()) { std::filesystem::remove_all(sqlDirectory); }
    std::filesystem::remove(databaseFile);
  }

  DbpFixture(const DbpFixture &) = delete;
  DbpFixture &operator=(const DbpFixture &) = delete;

  /** Add a second connection and rebuild the plugin.
   * @param name connection name, also the name of its SQL directory
   * @param connectionFile SQLite file the connection points to
   * @param isDefault whether the entry is marked as the default connection */
  void addConnection(const std::string &name, const std::string &connectionFile, bool isDefault)
  {
    entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
      connectionFile,
      "",
      "",
      db::ConnectionType::SQLite,
      name,
      -1,
      "",
      isDefault));
    build();
  }

  void writeFile(const std::string &fileName, const std::string &content)
  {
    dbpWriteFile(sqlDirectory, fileName, content);
  }

private:
  void build()
  {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(
      EnvironmentConfiguration::ConfigDirectory, std::filesystem::path("/tmp/nonexistent_dbp_cfg").string());
    configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
    configuration->setEntries(entries);
    connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
    plugin = std::make_shared<DatabaseBootstrapPlugin>(connectionConfigurations);
  }
};

}// namespace

TEST_CASE("DatabaseBootstrapPlugin: getPriority is Database", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("priority");

  REQUIRE(fixture.plugin->getPriority() == BootstrapSequence::Database);
}
TEST_CASE("DatabaseBootstrapPlugin: valid configuration creates the schema and the tracking row",
  "[db_bootstrap_plugin]")
{
  DbpFixture fixture("default_sqlite", "DEFAULT_SQLITE");

  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "users"));
  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // the shipped init file created every table of the default schema ...
  REQUIRE(dbpTableExists(fixture.databaseFile, "users"));
  REQUIRE(dbpTableExists(fixture.databaseFile, "groups"));
  REQUIRE(dbpTableExists(fixture.databaseFile, "message_queues"));
  REQUIRE(dbpTableExists(fixture.databaseFile, "shared_memory_repositories"));
  // ... and both files were recorded as the schema "default" at version 1
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "default") == 1);
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: a sql file starting with a comment is not executed", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("comment", "DEFAULT_SQLITE");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // executeInitFile strips every '\n' before it splits the file at ';', so the leading comment
  // of the shipped data file merges with the statement behind it: the insert into "users" is
  // swallowed, while the following inserts (which start on their own line) are executed. Pinned
  // here, because a fix would change what the shipped seed data inserts.
  REQUIRE(dbpTableExists(fixture.databaseFile, "users"));
  REQUIRE(dbpCount(fixture.databaseFile, "select user_name from users") == 0);
  REQUIRE(dbpCount(fixture.databaseFile, "select name from groups") == 2);
  REQUIRE(dbpCount(fixture.databaseFile, "select user_name, group_name from user_groups_relation") == 2);
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "default") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: a second bootstrap applies the files only once", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("idempotent");
  fixture.writeFile("init_schema_demo_version_1.sql", "create table demo (id integer);");
  fixture.writeFile("data_schema_demo_version_1.sql", "insert into demo values (1);");

  fixture.plugin->onStart();
  REQUIRE(dbpCount(fixture.databaseFile, "select id from demo") == 1);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // re-running must neither duplicate the data rows nor the tracking row
  REQUIRE(dbpCount(fixture.databaseFile, "select id from demo") == 1);
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 1);
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "demo") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: missing sql directory only creates the tracking table", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("missing_dir");
  std::filesystem::remove_all(fixture.sqlDirectory);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(dbpTableExists(fixture.databaseFile, "schema_version"));
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 0);
}

TEST_CASE("DatabaseBootstrapPlugin: init and data files are applied and recorded", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("init_and_data");
  fixture.writeFile("init_schema_demo_version_1.sql", "create table demo (id integer);");
  fixture.writeFile("data_schema_demo_version_1.sql", "insert into demo values (1);");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(dbpTableExists(fixture.databaseFile, "demo"));
  REQUIRE(dbpCount(fixture.databaseFile, "select id from demo") == 1);
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "demo") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: a higher version is rejected as a version jump", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("upgrade");
  fixture.writeFile("init_schema_demo_version_1.sql", "create table demo (id integer);");
  fixture.writeFile("data_schema_demo_version_1.sql", "insert into demo values (1);");
  fixture.plugin->onStart();
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "demo") == 1);

  fixture.writeFile("init_schema_demo_version_2.sql", "alter table demo add column extra text;");
  fixture.writeFile("data_schema_demo_version_2.sql", "insert into demo values (2);");
  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // initDatabase starts its counter at 0 instead of the recorded version, so the first file of an
  // upgrade (init version 2) trips the "version jump" guard. The migration is dropped silently
  // and the recorded version stays where it was. Pinned, because fixing the counter would change
  // the upgrade path of an already initialised database.
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "demo") == 1);
  REQUIRE(dbpCount(fixture.databaseFile, "select id from demo") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: a version jump aborts the whole directory", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("jump");
  // the schema map is ordered by name: "aaa_jump" is processed before "zzz_good"
  fixture.writeFile("init_schema_aaa_jump_version_2.sql", "create table jump (id integer);");
  fixture.writeFile("init_schema_zzz_good_version_1.sql", "create table good (id integer);");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "jump"));
  // the abort returns out of initDatabase, so the schemas behind the jump are skipped as well
  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "good"));
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 0);
}

TEST_CASE("DatabaseBootstrapPlugin: data file without a matching schema version is skipped", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("orphan_data");
  fixture.writeFile("data_schema_demo_version_2.sql",
    "create table demo (id integer);\n"
    "insert into demo values (1);");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "demo"));
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 0);
}

TEST_CASE("DatabaseBootstrapPlugin: files that carry no version are ignored", "[db_bootstrap_plugin]")
{
  DbpFixture fixture("unversioned");
  fixture.writeFile("init_schema_demo_version_1.sql", "create table demo (id integer);");
  // starts with init_ but has no _version_ suffix: versionOf() returns -1 and the file is
  // filtered out before it is executed
  fixture.writeFile("init_schema_unversioned.sql", "create table unversioned (id integer);");
  fixture.writeFile("readme.txt", "not a sql file at all");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(dbpTableExists(fixture.databaseFile, "demo"));
  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "unversioned"));
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 1);
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "demo") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: an unreachable database does not stop the other connections",
  "[db_bootstrap_plugin]")
{
  DbpFixture fixture("unreachable", "DEFAULT_SQLITE");
  // the directory of the connection does not exist, so sqlite cannot open the file
  const std::filesystem::path brokenFile = "/nonexistent_dbp_root/broken.db";
  // the map of connections is ordered by name, so the broken one comes first
  fixture.addConnection("AAA_broken", brokenFile.string(), false);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE_FALSE(std::filesystem::exists(brokenFile));
  REQUIRE(dbpTableExists(fixture.databaseFile, "users"));
  REQUIRE(dbpSchemaVersion(fixture.databaseFile, "default") == 1);
}

TEST_CASE("DatabaseBootstrapPlugin: the sql directory ignores the environment configuration", "[db_bootstrap_plugin]")
{
  // A complete configuration directory with sql files below <configDir>/database/<name>, which
  // the plugin would pick up if it used EnvironmentConfiguration instead of the macro.
  const std::string tag = dbpUnique("env_cfg");
  const auto configDir = std::filesystem::temp_directory_path() / tag;
  const auto sqlDir = configDir / "database" / "env_cfg_db";
  std::filesystem::create_directories(sqlDir);
  dbpWriteFile(sqlDir, "init_schema_env_version_1.sql", "create table env_demo (id integer);");

  // The connection is named after the file below configDir; below CONFIG_DIRECTORY there is no
  // directory of that name, so nothing can be applied there.
  DbpFixture fixture("env_cfg", "env_cfg_db", configDir);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // EnvironmentConfiguration::ConfigDirectory points at configDir, but the plugin keeps using
  // <CONFIG_DIRECTORY>/database/<name>, so the file above was never applied.
  REQUIRE_FALSE(dbpTableExists(fixture.databaseFile, "env_demo"));
  REQUIRE(dbpCount(fixture.databaseFile, "select name from schema_version") == 0);

  std::filesystem::remove_all(configDir);
}
