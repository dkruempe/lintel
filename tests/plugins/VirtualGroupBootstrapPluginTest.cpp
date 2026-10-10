#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "lintel/core/exceptions/SQLException.h"
#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/persistence/Result.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/plugins/VirtualGroupBootstrapPlugin.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/DatabaseConnectionComponent.h"
#include "lintel/features/base/configuration/DatabaseConnectionEntry.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/provider/GroupProvider.h"

#include <catch2/catch_all.hpp>

/**
 * VirtualGroupBootstrapPlugin writes the groups of every registered GroupProvider into the groups
 * table and links the base groups Admin and User to the group rows.
 *
 * The link is written by NAME PREFIX ("Admin..." / "User..."), not by a configuration flag - a
 * documented workaround that is pinned by "the admin and user base groups are attached by name
 * prefix" below, so that replacing it with a flag becomes a visible, conscious change.
 */

namespace {

/** A provider holding the given virtual group names. */
std::shared_ptr<GroupProvider> vgBootstrapProvider(const std::vector<std::string> &groupNames)
{
  auto provider = std::make_shared<GroupProvider>();
  for (const auto &name : groupNames) { provider->add(Group(name, {}, true)); }
  return provider;
}

/** @return the rows of the given query as "col0|col1" strings, in query order */
std::vector<std::string> vgBootstrapRows(const std::filesystem::path &databaseFile, const std::string &query)
{
  std::vector<std::string> rows;
  db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
  db::Statement statement(connection);
  db::Result result = statement.execute(query);
  for (std::size_t i = 0; i < static_cast<std::size_t>(result.getSize()); i++) {
    rows.push_back(result.getValue(static_cast<int>(i), 0));
  }
  return rows;
}

std::vector<std::string> vgBootstrapGroupNames(const std::filesystem::path &databaseFile)
{
  return vgBootstrapRows(databaseFile, "select name from groups order by name");
}

std::vector<std::string> vgBootstrapRelations(const std::filesystem::path &databaseFile)
{
  return vgBootstrapRows(databaseFile,
    "select group_name || '|' || base_group_name from group_groups_relation"
    " order by group_name, base_group_name");
}

struct VgBootstrapFixture
{
  std::filesystem::path databaseFile;
  std::shared_ptr<Configuration> configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
  std::shared_ptr<Hypodermic::Container> container;
  std::shared_ptr<VirtualGroupBootstrapPlugin> plugin;

  explicit VgBootstrapFixture(const std::vector<std::vector<std::string>> &providerGroups,
    bool withDefaultConnection = true,
    db::ConnectionType type = db::ConnectionType::SQLite)
  {
    static unsigned int counter = 0;
    databaseFile = std::filesystem::temp_directory_path()
                   / ("vg_bootstrap_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".db");
    std::filesystem::remove(databaseFile);

    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    statement.execute(R"(
            CREATE TABLE groups (
                name text NOT NULL,
                virtual bool NOT NULL,
                CONSTRAINT group_pk PRIMARY KEY (name)
            );
        )");
    statement.execute(R"(
            CREATE TABLE group_groups_relation (
                group_name text NOT NULL,
                base_group_name text NOT NULL,
                CONSTRAINT group_groups_relation_pk PRIMARY KEY (group_name, base_group_name)
            );
        )");

    std::vector<std::shared_ptr<Entry>> entries;
    entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
      databaseFile.string(),
      "",
      "",
      type,
      "default",
      -1,
      "",
      withDefaultConnection));
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/tmp/nonexistent_vg_bootstrap_cfg");
    envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
    configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
    configuration->setEntries(entries);
    connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);

    Hypodermic::ContainerBuilder builder;
    for (const auto &groups : providerGroups) { builder.registerInstance(vgBootstrapProvider(groups)); }
    container = builder.build();
    plugin = std::make_shared<VirtualGroupBootstrapPlugin>(connectionConfigurations, container);
  }

  ~VgBootstrapFixture() { std::filesystem::remove(databaseFile); }

  VgBootstrapFixture(const VgBootstrapFixture &) = delete;
  VgBootstrapFixture &operator=(const VgBootstrapFixture &) = delete;

  /** @return the virtual flag of the given group, "" if the group is unknown */
  std::string virtualFlagOf(const std::string &groupName)
  {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    db::Result result = statement.execute("select virtual from groups where name = '" + groupName + "'");
    return result.getSize() == 0 ? std::string{} : result.getValue(0, 0);
  }

  /** @return the number of rows of the given table */
  int countOf(const std::string &table)
  {
    db::Connection connection(db::ConnectionType::SQLite, databaseFile.string());
    db::Statement statement(connection);
    db::Result result = statement.execute("select count(*) from " + table);
    return result.getSize() == 0 ? -1 : std::stoi(result.getValue(0, 0));
  }
};

}// namespace

TEST_CASE("VirtualGroupBootstrapPlugin: getPriority is VirtualGroups", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin" } });

  REQUIRE(fixture.plugin->getPriority() == BootstrapSequence::VirtualGroups);
}

TEST_CASE("VirtualGroupBootstrapPlugin: without a default connection nothing is written", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin", "User" } }, false);

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.countOf("groups") == 0);
  REQUIRE(fixture.countOf("group_groups_relation") == 0);
}

TEST_CASE("VirtualGroupBootstrapPlugin: provided groups are written as virtual", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin", "User", "_PropertyGroupMember" } });

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(
    vgBootstrapGroupNames(fixture.databaseFile) == std::vector<std::string>{ "Admin", "User", "_PropertyGroupMember" });
  // sqlite serializes a true bool as "t"
  REQUIRE(fixture.virtualFlagOf("Admin") == "t");
  REQUIRE(fixture.virtualFlagOf("_PropertyGroupMember") == "t");
}

TEST_CASE("VirtualGroupBootstrapPlugin: the groups of every provider are collected", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin" }, { "User", "_PropertyGroupMember" } });

  fixture.plugin->onStart();

  REQUIRE(
    vgBootstrapGroupNames(fixture.databaseFile) == std::vector<std::string>{ "Admin", "User", "_PropertyGroupMember" });
  REQUIRE(fixture.countOf("groups") == 3);
}

TEST_CASE("VirtualGroupBootstrapPlugin: a group provided by two providers is written once", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin", "Shared" }, { "Shared", "User" } });

  fixture.plugin->onStart();

  REQUIRE(fixture.countOf("groups") == 3);
  REQUIRE(vgBootstrapGroupNames(fixture.databaseFile) == std::vector<std::string>{ "Admin", "Shared", "User" });
}

TEST_CASE("VirtualGroupBootstrapPlugin: the admin and user base groups are attached by name prefix",
  "[vg_bootstrap_plugin]")
{
  // DOCUMENTED WORKAROUND, NOT THE INTENDED DESIGN - see VirtualGroupBootstrapPlugin.cpp:
  // the base groups are attached by classifying the group name, so anything that starts with
  // "Admin" or "User" is treated as a collecting group and everything else is left alone.
  VgBootstrapFixture fixture({ { "Admin",
    "AdminX",
    "User",
    "UserY",
    "Auditors",
    "Superuser",
    "_PropertyGroupMember",
    "admin_lowercase",
    "user_lowercase" } });

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  // every provided group is persisted ...
  REQUIRE(fixture.countOf("groups") == 9);
  // ... and only the prefixed ones get a relation. The row is (base group, collected group).
  REQUIRE(vgBootstrapRelations(fixture.databaseFile)
          == std::vector<std::string>{ "Admin|Admin", "Admin|AdminX", "User|User", "User|UserY" });
  REQUIRE(fixture.countOf("group_groups_relation") == 4);
}

TEST_CASE("VirtualGroupBootstrapPlugin: the prefix classification is case sensitive", "[vg_bootstrap_plugin]")
{
  // "Auditors" contains but does not start with "Admin", "Superuser" does not start with "User",
  // and a lower case "admin" is not the prefix either.
  VgBootstrapFixture fixture({ { "Auditors", "Superuser", "admin_lowercase", "user_lowercase" } });

  fixture.plugin->onStart();

  REQUIRE(fixture.countOf("groups") == 4);
  REQUIRE(fixture.countOf("group_groups_relation") == 0);
}

TEST_CASE("VirtualGroupBootstrapPlugin: a group equal to a base name gets a self relation", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin", "User" } });

  fixture.plugin->onStart();

  REQUIRE(vgBootstrapRelations(fixture.databaseFile) == std::vector<std::string>{ "Admin|Admin", "User|User" });
}

TEST_CASE("VirtualGroupBootstrapPlugin: a second bootstrap does not duplicate any row", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin", "User", "AdminX" } });

  fixture.plugin->onStart();
  fixture.plugin->onStart();

  REQUIRE(fixture.countOf("groups") == 3);
  REQUIRE(fixture.countOf("group_groups_relation") == 3);
  REQUIRE(vgBootstrapRelations(fixture.databaseFile)
          == std::vector<std::string>{ "Admin|Admin", "Admin|AdminX", "User|User" });
}

TEST_CASE("VirtualGroupBootstrapPlugin: an existing group row is not overwritten", "[vg_bootstrap_plugin]")
{
  VgBootstrapFixture fixture({ { "Admin" } });
  {
    db::Connection connection(db::ConnectionType::SQLite, fixture.databaseFile.string());
    db::Statement statement(connection);
    statement.execute("insert into groups(name, virtual) values ('Admin', '0')");
  }

  fixture.plugin->onStart();

  // "insert or ignore" keeps the existing row including its virtual flag ...
  REQUIRE(fixture.countOf("groups") == 1);
  REQUIRE(fixture.virtualFlagOf("Admin") == "0");
  // ... while the relation is still written
  REQUIRE(vgBootstrapRelations(fixture.databaseFile) == std::vector<std::string>{ "Admin|Admin" });
}

TEST_CASE("VirtualGroupBootstrapPlugin: an undefined connection type fails in the transaction", "[vg_bootstrap_plugin]")
{
  // The switch in onStart() has a "throw unknown db type" branch, but db::Transaction rejects an
  // UNDEFINED connection before it is reached - pinned so the difference stays visible.
  VgBootstrapFixture fixture({ { "Admin" } }, true, db::ConnectionType::UNDEFINED);

  REQUIRE_THROWS_AS(fixture.plugin->onStart(), db::SQLException);
  REQUIRE(fixture.countOf("groups") == 0);
}
