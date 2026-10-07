#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/DatabaseConnectionComponent.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/repositories/GroupRepository.h>
#include <lintel/features/base/repositories/UserRepository.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::string kUserPagingDb() {
    static int counter = 0;
    return (std::filesystem::temp_directory_path() /
            ("user_paging_test_" + std::to_string(counter++) + ".db"))
            .string();
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct UserPagingFixture {
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_user_paging_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<UserRepository> userRepository;

    UserPagingFixture() : m_dbPath(kUserPagingDb()) {
        std::remove(m_dbPath.c_str());

        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
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

        envConfig = std::make_shared<EnvironmentConfiguration>();
        configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{}, envConfig);
        auto entry = std::make_shared<DatabaseConnectionEntry>(
                type_name<DatabaseConnectionComponent>(), m_dbPath, "", "",
                db::ConnectionType::SQLite, "default", 0, "", true);
        configuration->setEntries(
                std::vector<std::shared_ptr<Entry>>{entry});
        connectionConfigurations =
                std::make_shared<DatabaseConnectionConfigurations>(configuration);
        groupRepository = std::make_shared<GroupRepository>(connectionConfigurations);
        // createOf registers the group in both the database and the repository's
        // in-memory map, which of() resolves against.
        groupRepository->createOf(Group(std::string("g1"), {}, false));
        groupRepository->createOf(Group(std::string("g2"), {}, false));
        userRepository =
                std::make_shared<UserRepository>(connectionConfigurations, groupRepository);
    }

    ~UserPagingFixture() { std::remove(m_dbPath.c_str()); }

    void createUser(const std::string &name) {
        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
        db::ParameterBuilder builder(connectionConfigurations->ofDefault());
        builder.add(std::string(name)).add(std::string("pw"))
                .add(name + "@test.com").add(std::string("First"))
                .add(std::string("Last")).add(std::string("2026-01-01T00:00:00Z"));
        statement.execute(
                "insert into users (user_name, password, e_mail, first_name, "
                "last_name, created_timestamp) values (?, ?, ?, ?, ?, ?)",
                builder);
    }

    void assignGroup(const std::string &userName, const std::string &groupName) {
        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
        db::ParameterBuilder builder(connectionConfigurations->ofDefault());
        builder.add(userName).add(groupName);
        statement.execute(
                "insert into user_groups_relation (user_name, group_name) values (?, ?)",
                builder);
    }
};

}  // namespace

TEST_CASE("UserRepository::pageOf walks through all users", "[user_paging]") {
    UserPagingFixture fixture;
    for (const char *name : {"alice", "bob", "carol", "dave", "eve"}) {
        fixture.createUser(name);
    }

    const Page<User> page1 = fixture.userRepository->pageOf("", std::nullopt, 2);
    REQUIRE(page1.getItems().size() == 2);
    REQUIRE(page1.getItems()[0].getUserName() == "alice");
    REQUIRE(page1.getItems()[1].getUserName() == "bob");
    REQUIRE(page1.hasMore());
    REQUIRE(page1.getNextAfter().has_value());
    REQUIRE(page1.getNextAfter().value() == "bob");

    const Page<User> page2 =
            fixture.userRepository->pageOf("", page1.getNextAfter(), 2);
    REQUIRE(page2.getItems().size() == 2);
    REQUIRE(page2.getItems()[0].getUserName() == "carol");
    REQUIRE(page2.getItems()[1].getUserName() == "dave");
    REQUIRE(page2.hasMore());
    REQUIRE(page2.getNextAfter().value() == "dave");

    const Page<User> page3 =
            fixture.userRepository->pageOf("", page2.getNextAfter(), 2);
    REQUIRE(page3.getItems().size() == 1);
    REQUIRE(page3.getItems()[0].getUserName() == "eve");
    REQUIRE_FALSE(page3.hasMore());
    REQUIRE_FALSE(page3.getNextAfter().has_value());

    // walking past the end yields an empty page
    const Page<User> page4 = fixture.userRepository->pageOf("", std::string("zzz"), 2);
    REQUIRE(page4.getItems().empty());
    REQUIRE_FALSE(page4.hasMore());
    REQUIRE_FALSE(page4.getNextAfter().has_value());
}

TEST_CASE("UserRepository::pageOf keeps multi-group user on one page",
          "[user_paging]") {
    UserPagingFixture fixture;
    fixture.createUser("alpha");
    fixture.createUser("beta");
    fixture.createUser("gamma");
    for (const char *group : {"g1", "g2"}) {
        fixture.assignGroup("beta", group);
    }

    const Page<User> page = fixture.userRepository->pageOf("", std::nullopt, 2);
    REQUIRE(page.getItems().size() == 2);
    REQUIRE(page.getItems()[0].getUserName() == "alpha");
    REQUIRE(page.getItems()[1].getUserName() == "beta");
    REQUIRE(page.getItems()[1].getGroups().size() == 2);
}

TEST_CASE("UserRepository::pageOf respects name filter", "[user_paging]") {
    UserPagingFixture fixture;
    for (const char *name : {"admin1", "admin2", "viewer"}) {
        fixture.createUser(name);
    }

    const Page<User> page = fixture.userRepository->pageOf("admin.*", std::nullopt, 1);
    REQUIRE(page.getItems().size() == 1);
    REQUIRE(page.getItems()[0].getUserName() == "admin1");
    REQUIRE(page.hasMore());

    const Page<User> page2 = fixture.userRepository->pageOf("admin.*", page.getNextAfter(), 1);
    REQUIRE(page2.getItems().size() == 1);
    REQUIRE(page2.getItems()[0].getUserName() == "admin2");
    REQUIRE_FALSE(page2.hasMore());
}

TEST_CASE("UserRepository::pageOf returns empty page for unknown filter",
          "[user_paging]") {
    UserPagingFixture fixture;
    fixture.createUser("alice");

    const Page<User> page = fixture.userRepository->pageOf("nobody.*", std::nullopt, 10);
    REQUIRE(page.getItems().empty());
    REQUIRE_FALSE(page.hasMore());
    REQUIRE_FALSE(page.getNextAfter().has_value());
}

TEST_CASE("UserRepository::pageOf handles limit larger than dataset",
          "[user_paging]") {
    UserPagingFixture fixture;
    for (const char *name : {"alice", "bob"}) {
        fixture.createUser(name);
    }

    const Page<User> page = fixture.userRepository->pageOf("", std::nullopt, 1000);
    REQUIRE(page.getItems().size() == 2);
    REQUIRE_FALSE(page.hasMore());
    REQUIRE_FALSE(page.getNextAfter().has_value());
}
