#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/core/persistence/Statement.h>
#include <base_library/core/persistence/Transaction.h>
#include <base_library/core/utils/Cryption.h>
#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/DatabaseConnectionComponent.h>
#include <base_library/features/base/configuration/DatabaseConnectionEntry.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/models/ProcessName.h>
#include <base_library/features/base/models/User.h>
#include <base_library/features/base/repositories/GroupRepository.h>
#include <base_library/features/base/repositories/UserRepository.h>
#include <base_library/features/base/services/AuthService.h>
#include <base_library/features/base/services/SchedulerService.h>

#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::string kTestDb() {
    return (std::filesystem::temp_directory_path() /
            ("auth_service_test_" + std::to_string(getpid()) + ".db")).string();
}

// m_configDirectory must be declared before m_dbPath: members initialize in declaration order, so
// the guard sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration,
// and it is destroyed last - after the test case is done with it.
struct AuthFixture {
    AuthFixture() {
        m_dbPath = kTestDb();
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
        groupRepository =
                std::make_shared<GroupRepository>(connectionConfigurations);
        userRepository =
                std::make_shared<UserRepository>(connectionConfigurations,
                                                 groupRepository);
        processName = std::make_shared<ProcessName>("test-process");
        scheduler = std::make_shared<SchedulerService>(processName);
        authService =
                std::make_shared<AuthService>(processName, userRepository,
                                              scheduler);
    }

    ~AuthFixture() {
        authService->onShutdown();
        scheduler->onShutdown();
        std::remove(m_dbPath.c_str());
    }

    void createUser(const std::string &userName, const std::string &password) {
        User user("First", "Last", User::Sex::Male, "", userName,
                  Cryption::hashOf(password), {});
        userRepository->createOf(user);
    }

  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_authtest_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<UserRepository> userRepository;
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<AuthService> authService;
};

}  // namespace

TEST_CASE("AuthService: allTokensOf returns only the sessions of the user") {
    AuthFixture fixture;
    fixture.createUser("alice", "secret1");
    fixture.createUser("bob", "secret2");

    auto token1 = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.1", "alice", "secret1"});
    auto token2 = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.1", "alice", "secret1"});
    auto tokenBob = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.2", "bob", "secret2"});

    REQUIRE(token1.has_value());
    REQUIRE(token2.has_value());
    REQUIRE(tokenBob.has_value());

    auto aliceSessions = fixture.authService->allTokensOf("alice");
    REQUIRE(aliceSessions.size() == 2);
    REQUIRE(aliceSessions[0].m_user.getUserName() == "alice");
    REQUIRE(aliceSessions[1].m_user.getUserName() == "alice");

    auto bobSessions = fixture.authService->allTokensOf("bob");
    REQUIRE(bobSessions.size() == 1);
    REQUIRE(bobSessions[0].m_id == tokenBob->m_id);
}

TEST_CASE("AuthService: revokeTokenOf removes a session") {
    AuthFixture fixture;
    fixture.createUser("alice", "secret1");

    auto token = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.1", "alice", "secret1"});
    REQUIRE(token.has_value());

    fixture.authService->revokeTokenOf(token->m_id);
    REQUIRE(fixture.authService->allTokensOf("alice").empty());

    auto access = fixture.authService->onAccessOf(
            UserTokenLogin{"192.168.1.1", token->m_id});
    REQUIRE_FALSE(access.has_value());
}

TEST_CASE("AuthService: onLogoutOf removes a session") {
    AuthFixture fixture;
    fixture.createUser("alice", "secret1");

    auto token = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.1", "alice", "secret1"});
    REQUIRE(token.has_value());

    fixture.authService->onLogoutOf(
            UserTokenLogin{"192.168.1.1", token->m_id});
    REQUIRE(fixture.authService->allTokensOf("alice").empty());
}

TEST_CASE("AuthService: sliding window rate limits excessive logins per IP") {
    AuthFixture fixture;
    fixture.createUser("alice", "secret1");

    // default window allows 20 login attempts per IP
    for (int i = 0; i < 20; i++) {
        auto token = fixture.authService->onLoginOf(
                UserLogin{"192.168.1.1", "alice", "secret1"});
        REQUIRE(token.has_value());
    }
    auto blocked = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.1", "alice", "secret1"});
    REQUIRE_FALSE(blocked.has_value());

    // a different IP is not affected
    auto otherIp = fixture.authService->onLoginOf(
            UserLogin{"192.168.1.2", "alice", "secret1"});
    REQUIRE(otherIp.has_value());
}
