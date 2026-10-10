#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/persistence/Result.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/plugins/AdminUserBootstrapPlugin.h"
#include "lintel/core/utils/Cryption.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/DatabaseConnectionComponent.h"
#include "lintel/features/base/configuration/DatabaseConnectionEntry.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

/**
 * AdminUserBootstrapPlugin creates the initial admin user from ADMIN_USERNAME / ADMIN_PASSWORD.
 *
 * Without ADMIN_PASSWORD no user is created at all - the plugin never falls back to a default
 * password. Everything it does happens in one transaction, and every failure is logged and
 * swallowed, so onStart() itself never throws.
 */

namespace {

struct AdminUserFixture
{
  std::filesystem::path databaseFile;
  /** database the connection entry of the plugin points to */
  std::filesystem::path connectionPath;
  std::shared_ptr<Configuration> configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
  std::shared_ptr<AdminUserBootstrapPlugin> plugin;

  /** @param isDefault whether the connection is marked as the default connection
   *  @param database database the entry points to, empty for the file of the fixture
   *  @param withUsersTable whether the users table exists
   *  @param withAdminGroup whether the Admin group exists */
  explicit AdminUserFixture(bool isDefault = true,
    const std::string &database = {},
    bool withUsersTable = true,
    bool withAdminGroup = true)
  {
    static unsigned int counter = 0;
    databaseFile = std::filesystem::temp_directory_path()
                   / ("admin_user_bootstrap_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".db");
    std::filesystem::remove(databaseFile);
    connectionPath = database.empty() ? databaseFile : std::filesystem::path(database);

    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    if (withUsersTable) { createUsersTable(statement); }
    statement.execute(R"(
            CREATE TABLE groups (
                name text NOT NULL,
                virtual bool NOT NULL,
                CONSTRAINT group_pk PRIMARY KEY (name)
            );
        )");
    statement.execute(R"(
            CREATE TABLE user_groups_relation (
                user_name text NOT NULL,
                group_name text NOT NULL,
                CONSTRAINT user_groups_pk PRIMARY KEY (user_name, group_name)
            );
        )");
    if (withAdminGroup) { statement.execute("insert into groups(name, virtual) values ('Admin', '0')"); }

    std::vector<std::shared_ptr<Entry>> entries;
    entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
      connectionPath.string(),
      "",
      "",
      db::ConnectionType::SQLite,
      "default",
      -1,
      "",
      isDefault));
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/tmp/nonexistent_admin_user_cfg");
    envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
    configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
    configuration->setEntries(entries);
    connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
    plugin = std::make_shared<AdminUserBootstrapPlugin>(connectionConfigurations);
  }

  ~AdminUserFixture() { std::filesystem::remove(databaseFile); }

  AdminUserFixture(const AdminUserFixture &) = delete;
  AdminUserFixture &operator=(const AdminUserFixture &) = delete;

  static void createUsersTable(db::Statement &statement)
  {
    statement.execute(R"(
            CREATE TABLE users (
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                e_mail TEXT NOT NULL,
                user_name TEXT NOT NULL,
                password TEXT NOT NULL,
                created_timestamp TEXT NOT NULL,
                CONSTRAINT user_pk PRIMARY KEY (user_name)
            );
        )");
  }

  /** Creates the missing users table, e.g. after a failed bootstrap. */
  void createUsersTable()
  {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    createUsersTable(statement);
  }

  void insertUser(const std::string &userName, const std::string &password)
  {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    statement.execute(
      "insert into users(first_name, last_name, e_mail, user_name, password, "
      "created_timestamp) values ('First', 'Last', 'mail@test.com', '"
      + userName + "', '" + password + "', '2021-01-01 00:00:00.000000+0000')");
  }

  /** @return value of the single column of the first row of the query, "" if there is none */
  std::string scalar(const std::string &query)
  {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    db::Result result = statement.execute(query);
    return result.getSize() == 0 ? std::string{} : result.getValue(0, 0);
  }

  int countOf(const std::string &table)
  {
    const std::string value = scalar("select count(*) from " + table);
    return value.empty() ? -1 : std::stoi(value);
  }
};

}// namespace

TEST_CASE("AdminUserBootstrapPlugin: getPriority is AdminUser", "[admin_user_bootstrap_plugin]")
{
  AdminUserFixture fixture;

  REQUIRE(fixture.plugin->getPriority() == BootstrapSequence::AdminUser);
}

TEST_CASE("AdminUserBootstrapPlugin: without ADMIN_PASSWORD no user is created", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", std::nullopt };
  ScopedEnvironmentVariable userName{ "ADMIN_USERNAME", "ignored" };
  AdminUserFixture fixture;

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("users") == 0);
  REQUIRE(fixture.countOf("user_groups_relation") == 0);
}

TEST_CASE("AdminUserBootstrapPlugin: an empty ADMIN_PASSWORD counts as unset", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "" };
  AdminUserFixture fixture;

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("users") == 0);
}

TEST_CASE("AdminUserBootstrapPlugin: the admin user and its group relation are created",
  "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  ScopedEnvironmentVariable userName{ "ADMIN_USERNAME", std::nullopt };
  AdminUserFixture fixture;

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("users") == 1);
  REQUIRE(fixture.scalar("select user_name from users") == "admin");
  REQUIRE(fixture.scalar("select group_name from user_groups_relation") == "Admin");
  REQUIRE(fixture.countOf("user_groups_relation") == 1);
  // the user is related to the Admin group only, not to a "User" group
  REQUIRE(fixture.countOf("user_groups_relation where group_name = 'User'") == 0);
  REQUIRE(fixture.scalar("select created_timestamp from users").empty() == false);
  // e_mail is left empty, first_name and last_name carry the group names of the insert statement
  REQUIRE(fixture.scalar("select e_mail from users").empty());
  REQUIRE(fixture.scalar("select first_name from users") == "Admin");
  REQUIRE(fixture.scalar("select last_name from users") == "User");
}

TEST_CASE("AdminUserBootstrapPlugin: ADMIN_USERNAME names the created user", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  ScopedEnvironmentVariable userName{ "ADMIN_USERNAME", "operator" };
  AdminUserFixture fixture;

  fixture.plugin->onStart();

  REQUIRE(fixture.scalar("select user_name from users") == "operator");
  REQUIRE(fixture.scalar("select user_name from user_groups_relation") == "operator");
}

TEST_CASE("AdminUserBootstrapPlugin: the password is stored hashed", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  AdminUserFixture fixture;

  fixture.plugin->onStart();

  const std::string stored = fixture.scalar("select password from users");
  REQUIRE_FALSE(stored.empty());
  REQUIRE(stored != "s3cret-passphrase");
  REQUIRE(Cryption::isModernHash(stored));
  REQUIRE(Cryption::verifyOf("s3cret-passphrase", stored));
  REQUIRE_FALSE(Cryption::verifyOf("wrong-passphrase", stored));
}

TEST_CASE("AdminUserBootstrapPlugin: a second bootstrap duplicates nothing", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  AdminUserFixture fixture;

  fixture.plugin->onStart();
  const std::string hashOfFirstRun = fixture.scalar("select password from users");
  REQUIRE_NOTHROW(fixture.plugin->onStart());
  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("users") == 1);
  REQUIRE(fixture.countOf("user_groups_relation") == 1);
  REQUIRE(fixture.scalar("select password from users") == hashOfFirstRun);
}

TEST_CASE("AdminUserBootstrapPlugin: a second bootstrap does not change the password", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable userName{ "ADMIN_USERNAME", "operator" };
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "first-passphrase" };
  AdminUserFixture fixture;
  fixture.plugin->onStart();
  const std::string hashOfFirstRun = fixture.scalar("select password from users");
  REQUIRE(Cryption::verifyOf("first-passphrase", hashOfFirstRun));

  ScopedEnvironmentVariable otherPassword{ "ADMIN_PASSWORD", "second-passphrase" };
  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // the user exists, so the plugin skips instead of resetting the password
  REQUIRE(fixture.scalar("select password from users") == hashOfFirstRun);
  REQUIRE(Cryption::verifyOf("first-passphrase", fixture.scalar("select password from users")));
}

TEST_CASE("AdminUserBootstrapPlugin: an existing user is not touched", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  AdminUserFixture fixture;
  fixture.insertUser("admin", "already-there");

  fixture.plugin->onStart();

  REQUIRE(fixture.countOf("users") == 1);
  REQUIRE(fixture.scalar("select password from users") == "already-there");
  REQUIRE(fixture.scalar("select e_mail from users") == "mail@test.com");
  REQUIRE(fixture.countOf("user_groups_relation") == 0);
}

TEST_CASE("AdminUserBootstrapPlugin: without the Admin group no user is created", "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  AdminUserFixture fixture(true, "", true, false);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("groups") == 0);
  REQUIRE(fixture.countOf("users") == 0);
  REQUIRE(fixture.countOf("user_groups_relation") == 0);
}

TEST_CASE("AdminUserBootstrapPlugin: without a default connection no database is opened",
  "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  // the entry is not the default connection and points at a database that does not exist yet
  const std::string unreachable = "/tmp/admin_user_bootstrap_no_default_connection.db";
  std::filesystem::remove(unreachable);
  AdminUserFixture fixture(false, unreachable);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE_FALSE(std::filesystem::exists(fixture.connectionPath));
  std::filesystem::remove(unreachable);
}

TEST_CASE("AdminUserBootstrapPlugin: an unreachable database does not abort the bootstrap",
  "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  AdminUserFixture fixture(true, "/nonexistent_admin_user_root/admin.db");

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE_FALSE(std::filesystem::exists("/nonexistent_admin_user_root/admin.db"));
}

TEST_CASE("AdminUserBootstrapPlugin: a failing statement leaves the database unchanged and recoverable",
  "[admin_user_bootstrap_plugin]")
{
  ScopedEnvironmentVariable password{ "ADMIN_PASSWORD", "s3cret-passphrase" };
  // the users table does not exist, so the first statement of the plugin fails
  AdminUserFixture fixture(true, "", false);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("groups") == 1);
  REQUIRE(fixture.countOf("user_groups_relation") == 0);

  // the failure was only logged: once the table exists the very next bootstrap creates the user
  fixture.createUsersTable();
  REQUIRE_NOTHROW(fixture.plugin->onStart());
  REQUIRE(fixture.countOf("users") == 1);
  REQUIRE(fixture.scalar("select user_name from users") == "admin");
}
