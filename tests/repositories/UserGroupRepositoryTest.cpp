#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/core/persistence/Statement.h>
#include <base_library/core/persistence/Transaction.h>
#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/DatabaseConnectionComponent.h>
#include <base_library/features/base/configuration/DatabaseConnectionEntry.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/models/Group.h>
#include <base_library/features/base/models/User.h>
#include <base_library/features/base/repositories/GroupRepository.h>
#include <base_library/features/base/repositories/UserRepository.h>

#include <catch2/catch_all.hpp>

#include <boost/process/v1/environment.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::string kRepoTestDb() {
    static int counter = 0;
    return (std::filesystem::temp_directory_path() /
            ("user_group_repo_test_" +
             std::to_string(boost::this_process::get_id()) + "_" +
             std::to_string(++counter) + ".db"))
            .string();
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct RepoFixture {
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_repo_test_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<UserRepository> userRepository;

    RepoFixture() {
        m_dbPath = kRepoTestDb();
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
        statement.execute(R"(
            CREATE TABLE group_groups_relation (
                group_name text NOT NULL,
                base_group_name text NOT NULL,
                CONSTRAINT group_groups_relation_pk PRIMARY KEY (group_name, base_group_name)
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
        groupRepository =
                std::make_shared<GroupRepository>(connectionConfigurations);
        userRepository = std::make_shared<UserRepository>(
                connectionConfigurations, groupRepository);
    }

    ~RepoFixture() { std::remove(m_dbPath.c_str()); }

    Group makeGroup(const std::string &name, bool isVirtual = false) {
        return Group(name, {}, isVirtual);
    }

    User makeUser(const std::string &name, const std::string &email = "test@test.com") {
        return User("First", "Last", User::Sex::Male, email, name, "hash123",
                    std::vector<Group>{});
    }

    void insertVirtualGroup(const std::string &name) {
        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
        db::ParameterBuilder builder(connectionConfigurations->ofDefault());
        builder.add(name).add(true);
        statement.execute("INSERT INTO groups(name, virtual) VALUES(?, ?)", builder);
    }
};

}  // namespace

TEST_CASE("GroupRepository::createOf creates a group", "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");

    fixture.groupRepository->createOf(group);

    auto found = fixture.groupRepository->of("admins");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroupName() == "admins");
    REQUIRE_FALSE(found->isVirtual());
}

TEST_CASE("GroupRepository::createOf rejects virtual group", "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("virtual_group", true);

    REQUIRE_THROWS_AS(fixture.groupRepository->createOf(group), std::runtime_error);
}

TEST_CASE("GroupRepository::createOf rejects duplicate group", "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");

    fixture.groupRepository->createOf(group);
    REQUIRE_THROWS_AS(fixture.groupRepository->createOf(group), std::runtime_error);
}

TEST_CASE("GroupRepository::of returns nullopt for unknown group",
          "[group_repository]") {
    RepoFixture fixture;
    auto found = fixture.groupRepository->of("nonexistent");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("GroupRepository::allOf returns all groups", "[group_repository]") {
    RepoFixture fixture;
    fixture.groupRepository->createOf(fixture.makeGroup("admins"));
    fixture.groupRepository->createOf(fixture.makeGroup("users"));

    auto groups = fixture.groupRepository->allOf();
    REQUIRE(groups.size() == 2);
}

TEST_CASE("GroupRepository::allOf by virtual flag", "[group_repository]") {
    RepoFixture fixture;
    fixture.groupRepository->createOf(fixture.makeGroup("admins"));
    fixture.groupRepository->createOf(fixture.makeGroup("users"));

    auto realGroups = fixture.groupRepository->allOf(false);
    REQUIRE(realGroups.size() == 2);

    auto virtualGroups = fixture.groupRepository->allOf(true);
    REQUIRE(virtualGroups.empty());
}

TEST_CASE("GroupRepository::allOf by regex", "[group_repository]") {
    RepoFixture fixture;
    fixture.groupRepository->createOf(fixture.makeGroup("admins"));
    fixture.groupRepository->createOf(fixture.makeGroup("users"));
    fixture.groupRepository->createOf(fixture.makeGroup("admins2"));

    std::string pattern("admins.*");
    auto matched = fixture.groupRepository->allOf(pattern);
    REQUIRE(matched.size() == 2);
}

TEST_CASE("GroupRepository::allOf by regex and virtual flag", "[group_repository]") {
    RepoFixture fixture;
    fixture.groupRepository->createOf(fixture.makeGroup("admins"));
    fixture.groupRepository->createOf(fixture.makeGroup("users"));

    std::string pattern("admins");
    auto matched = fixture.groupRepository->allOf(pattern, false);
    REQUIRE(matched.size() == 1);
    REQUIRE(matched[0].getGroupName() == "admins");
}

TEST_CASE("GroupRepository::deleteOf removes group", "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);

    fixture.groupRepository->deleteOf(group);

    auto found = fixture.groupRepository->of("admins");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("GroupRepository::deleteOf fails when group is in use by user",
          "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("admin_user");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupOf(user, group);

    REQUIRE_THROWS_AS(fixture.groupRepository->deleteOf(group), std::runtime_error);
}

TEST_CASE("GroupRepository::addGroupOf adds virtual sub-group", "[group_repository]") {
    RepoFixture fixture;
    fixture.insertVirtualGroup("_PropertyGroupMember");
    fixture.groupRepository->onAwake();

    auto adminGroup = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(adminGroup);
    fixture.groupRepository->onAwake();

    auto baseGroup = fixture.groupRepository->of("_PropertyGroupMember");
    REQUIRE(baseGroup.has_value());
    auto admins = fixture.groupRepository->of("admins");
    REQUIRE(admins.has_value());

    fixture.groupRepository->addGroupOf(admins.value(), baseGroup.value());

    auto updated = fixture.groupRepository->of("admins");
    REQUIRE(updated.has_value());
    REQUIRE(updated->getGroups().size() == 1);
    REQUIRE(updated->getGroups()[0].getGroupName() == "_PropertyGroupMember");
}

TEST_CASE("GroupRepository::addGroupOf ignores non-virtual group", "[group_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);
    fixture.groupRepository->onAwake();

    auto realGroup = fixture.makeGroup("users");
    fixture.groupRepository->createOf(realGroup);

    auto admins = fixture.groupRepository->of("admins");
    fixture.groupRepository->addGroupOf(admins.value(), realGroup);

    auto updated = fixture.groupRepository->of("admins");
    REQUIRE(updated.has_value());
    REQUIRE(updated->getGroups().empty());
}

TEST_CASE("GroupRepository::addGroupOf ignores duplicate sub-group", "[group_repository]") {
    RepoFixture fixture;
    fixture.insertVirtualGroup("_PropertyGroupMember");
    fixture.groupRepository->onAwake();

    auto adminGroup = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(adminGroup);
    fixture.groupRepository->onAwake();

    auto baseGroup = fixture.groupRepository->of("_PropertyGroupMember");
    auto admins = fixture.groupRepository->of("admins");

    fixture.groupRepository->addGroupOf(admins.value(), baseGroup.value());
    fixture.groupRepository->addGroupOf(admins.value(), baseGroup.value());

    auto updated = fixture.groupRepository->of("admins");
    REQUIRE(updated.has_value());
    REQUIRE(updated->getGroups().size() == 1);
}

TEST_CASE("GroupRepository::removeGroupOf removes sub-group", "[group_repository]") {
    RepoFixture fixture;
    fixture.insertVirtualGroup("_PropertyGroupMember");
    fixture.groupRepository->onAwake();

    auto adminGroup = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(adminGroup);
    fixture.groupRepository->onAwake();

    auto baseGroup = fixture.groupRepository->of("_PropertyGroupMember");
    auto admins = fixture.groupRepository->of("admins");

    fixture.groupRepository->addGroupOf(admins.value(), baseGroup.value());

    auto updatedAdmins = fixture.groupRepository->of("admins");
    fixture.groupRepository->removeGroupOf(updatedAdmins.value(), baseGroup.value());

    auto finalAdmins = fixture.groupRepository->of("admins");
    REQUIRE(finalAdmins.has_value());
    REQUIRE(finalAdmins->getGroups().empty());
}

TEST_CASE("GroupRepository::onAwake loads groups from database",
          "[group_repository]") {
    RepoFixture fixture;
    fixture.groupRepository->createOf(fixture.makeGroup("admins"));
    fixture.groupRepository->createOf(fixture.makeGroup("users"));

    auto freshRepo = std::make_shared<GroupRepository>(
            fixture.connectionConfigurations);
    freshRepo->onAwake();

    auto groups = freshRepo->allOf();
    REQUIRE(groups.size() == 2);
}

TEST_CASE("UserRepository::createOf creates a user", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");

    fixture.userRepository->createOf(user);

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getUserName() == "john");
    REQUIRE(found->getFirstName() == "First");
    REQUIRE(found->getLastName() == "Last");
    REQUIRE(found->getEmail() == "test@test.com");
}

TEST_CASE("UserRepository::of returns nullopt for unknown user",
          "[user_repository]") {
    RepoFixture fixture;
    auto found = fixture.userRepository->of("nonexistent");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("UserRepository::allOf returns all users", "[user_repository]") {
    RepoFixture fixture;
    fixture.userRepository->createOf(fixture.makeUser("alice"));
    fixture.userRepository->createOf(fixture.makeUser("bob"));

    auto users = fixture.userRepository->allOf();
    REQUIRE(users.size() == 2);
}

TEST_CASE("UserRepository::allOf with regex filter", "[user_repository]") {
    RepoFixture fixture;
    fixture.userRepository->createOf(fixture.makeUser("admin_alice"));
    fixture.userRepository->createOf(fixture.makeUser("admin_bob"));
    fixture.userRepository->createOf(fixture.makeUser("user_charlie"));

    std::string pattern("admin_.*");
    auto admins = fixture.userRepository->allOf(pattern);
    REQUIRE(admins.size() == 2);
}

TEST_CASE("UserRepository::deleteOf removes user", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->deleteOf(user);

    auto found = fixture.userRepository->of("john");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("UserRepository::deleteOf removes user and relations",
          "[user_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupOf(user, group);

    fixture.userRepository->deleteOf(user);

    auto found = fixture.userRepository->of("john");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("UserRepository::deleteOf by username vector", "[user_repository]") {
    RepoFixture fixture;
    fixture.userRepository->createOf(fixture.makeUser("alice"));

    fixture.userRepository->deleteOf(std::vector<std::string>{"alice"});

    REQUIRE_FALSE(fixture.userRepository->of("alice").has_value());
}

TEST_CASE("UserRepository::addGroupOf adds group to user", "[user_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupOf(user, group);

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroups().size() == 1);
    REQUIRE(found->getGroups()[0].getGroupName() == "admins");
}

TEST_CASE("UserRepository::addGroupsOf adds multiple groups", "[user_repository]") {
    RepoFixture fixture;
    auto g1 = fixture.makeGroup("admins");
    auto g2 = fixture.makeGroup("users");
    fixture.groupRepository->createOf(g1);
    fixture.groupRepository->createOf(g2);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupsOf(user, std::set<Group>{g1, g2});

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroups().size() == 2);
}

TEST_CASE("UserRepository::removeGroupOf removes group from user",
          "[user_repository]") {
    RepoFixture fixture;
    auto group = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(group);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupOf(user, group);

    auto fetched = fixture.userRepository->of("john");
    REQUIRE(fetched.has_value());
    fixture.userRepository->removeGroupOf(fetched.value(), group);

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroups().empty());
}

TEST_CASE("UserRepository::removeGroupsOf removes multiple groups",
          "[user_repository]") {
    RepoFixture fixture;
    auto g1 = fixture.makeGroup("admins");
    auto g2 = fixture.makeGroup("users");
    fixture.groupRepository->createOf(g1);
    fixture.groupRepository->createOf(g2);
    fixture.groupRepository->onAwake();

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupsOf(user, std::set<Group>{g1, g2});

    auto fetched = fixture.userRepository->of("john");
    REQUIRE(fetched.has_value());
    fixture.userRepository->removeGroupsOf(fetched.value(), std::set<Group>{g1, g2});

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroups().empty());
}

TEST_CASE("UserRepository::changeFirstNameOf", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->changeFirstNameOf(user, "Jane");

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getFirstName() == "Jane");
}

TEST_CASE("UserRepository::changeLastNameOf", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->changeLastNameOf(user, "Doe");

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getLastName() == "Doe");
}

TEST_CASE("UserRepository::changeUserNameOf", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->changeUserNameOf(user, "jane");

    auto found = fixture.userRepository->of("jane");
    REQUIRE(found.has_value());
    REQUIRE(found->getUserName() == "jane");

    auto oldFound = fixture.userRepository->of("john");
    REQUIRE_FALSE(oldFound.has_value());
}

TEST_CASE("UserRepository::changePasswordOf", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->changePasswordOf(user, "new_hash");

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getPassword() == "new_hash");
}

TEST_CASE("UserRepository::changeEMailOf", "[user_repository]") {
    RepoFixture fixture;
    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);

    fixture.userRepository->changeEMailOf(user, "new@email.com");

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getEmail() == "new@email.com");
}

TEST_CASE("UserRepository with virtual group", "[user_repository]") {
    RepoFixture fixture;
    fixture.insertVirtualGroup("_PropertyGroupMember");
    fixture.groupRepository->onAwake();

    auto adminGroup = fixture.makeGroup("admins");
    fixture.groupRepository->createOf(adminGroup);
    fixture.groupRepository->onAwake();

    auto virtualGroup = fixture.groupRepository->of("_PropertyGroupMember");
    auto admins = fixture.groupRepository->of("admins");
    fixture.groupRepository->addGroupOf(admins.value(), virtualGroup.value());

    auto user = fixture.makeUser("john");
    fixture.userRepository->createOf(user);
    fixture.userRepository->addGroupOf(user, admins.value());

    auto found = fixture.userRepository->of("john");
    REQUIRE(found.has_value());
    REQUIRE(found->getGroups().size() == 1);
    REQUIRE(found->getGroups()[0].getGroupName() == "admins");
}
