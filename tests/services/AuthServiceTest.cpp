#include <lintel/core/configuration/Configuration.h>
#include <lintel/core/configuration/DatabaseConnectionComponent.h>
#include <lintel/core/configuration/DatabaseConnectionEntry.h>
#include <lintel/core/configuration/EnvironmentConfiguration.h>
#include <lintel/core/models/ProcessName.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/persistence/Transaction.h>
#include <lintel/core/utils/Cryption.h>
#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/repositories/GroupRepository.h>
#include <lintel/features/base/repositories/UserRepository.h>
#include <lintel/features/base/services/AuthService.h>
#include <lintel/features/base/services/SchedulerService.h>

#include <catch2/catch_all.hpp>

#include "../helpers/PlatformCompat.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  std::string kTestDb()
  {
    return (std::filesystem::temp_directory_path() / ("auth_service_test_" + std::to_string(getpid()) + ".db"))
      .string();
  }

  // m_configDirectory must be declared before m_dbPath: members initialize in declaration order, so
  // the guard sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration,
  // and it is destroyed last - after the test case is done with it.
  struct AuthFixture
  {
    AuthFixture()
    {
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
      configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
      auto entry = std::make_shared<DatabaseConnectionEntry>(
        type_name<DatabaseConnectionComponent>(), m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
      configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ entry });
      connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
      groupRepository = std::make_shared<GroupRepository>(connectionConfigurations);
      userRepository = std::make_shared<UserRepository>(connectionConfigurations, groupRepository);
      processName = std::make_shared<ProcessName>("test-process");
      scheduler = std::make_shared<SchedulerService>(processName);
      authService = std::make_shared<AuthService>(processName, userRepository, scheduler);
    }

    ~AuthFixture()
    {
      authService->onShutdown();
      scheduler->onShutdown();
      std::remove(m_dbPath.c_str());
    }

    void createUser(const std::string &userName, const std::string &password)
    {
      User user("First", "Last", User::Sex::Male, "", userName, Cryption::hashOf(password), {});
      userRepository->createOf(user);
    }

    void createUserWithStoredHash(const std::string &userName, const std::string &storedHash)
    {
      User user("First", "Last", User::Sex::Male, "", userName, storedHash, {});
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

}// namespace

TEST_CASE("AuthService: allTokensOf returns only the sessions of the user")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");
  fixture.createUser("bob", "secret2");

  auto token1 = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  auto token2 = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  auto tokenBob = fixture.authService->onLoginOf(UserLogin{ "192.168.1.2", "bob", "secret2" });

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

TEST_CASE("AuthService: revokeTokenOf removes a session")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  fixture.authService->revokeTokenOf(token->m_id);
  REQUIRE(fixture.authService->allTokensOf("alice").empty());

  auto access = fixture.authService->onAccessOf(UserTokenLogin{ "192.168.1.1", token->m_id });
  REQUIRE_FALSE(access.has_value());
}

TEST_CASE("AuthService: onLogoutOf removes a session")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  fixture.authService->onLogoutOf(UserTokenLogin{ "192.168.1.1", token->m_id });
  REQUIRE(fixture.authService->allTokensOf("alice").empty());
}

TEST_CASE("AuthService: sliding window rate limits excessive logins per IP")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  // default window allows 20 login attempts per IP
  for (int i = 0; i < 20; i++) {
    auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
    REQUIRE(token.has_value());
  }
  auto blocked = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE_FALSE(blocked.has_value());

  // a different IP is not affected
  auto otherIp = fixture.authService->onLoginOf(UserLogin{ "192.168.1.2", "alice", "secret1" });
  REQUIRE(otherIp.has_value());
}

TEST_CASE("AuthService: onLoginOf rejects an empty ip address")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "", "alice", "secret1" });
  REQUIRE_FALSE(token.has_value());
}

TEST_CASE("AuthService: onLoginOf rejects a malformed ip address")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "not-an-ip", "alice", "secret1" });
  REQUIRE_FALSE(token.has_value());
}

TEST_CASE("AuthService: onLoginOf rejects an unknown user")
{
  AuthFixture fixture;

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "ghost", "secret1" });
  REQUIRE_FALSE(token.has_value());
}

TEST_CASE("AuthService: onLoginOf rejects a wrong password and clears failures on success")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  for (int i = 0; i < 3; i++) {
    auto failed = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "wrong" });
    REQUIRE_FALSE(failed.has_value());
  }

  // a correct login below the lockout threshold clears the recorded failures
  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());
}

TEST_CASE("AuthService: repeated failures lock out an ip address")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  // default maxLoginFailures is 5
  for (int i = 0; i < 5; i++) {
    auto failed = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "wrong" });
    REQUIRE_FALSE(failed.has_value());
  }

  // even the correct password is now blocked by the lockout
  auto blocked = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE_FALSE(blocked.has_value());

  // a different ip is unaffected
  auto otherIp = fixture.authService->onLoginOf(UserLogin{ "192.168.1.2", "alice", "secret1" });
  REQUIRE(otherIp.has_value());
}

TEST_CASE("AuthService: onAccessOf returns the token and refreshes its timestamp")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  auto access = fixture.authService->onAccessOf(UserTokenLogin{ "192.168.1.1", token->m_id });
  REQUIRE(access.has_value());
  REQUIRE(access->m_id == token->m_id);
  REQUIRE(access->m_user.getUserName() == "alice");
}

TEST_CASE("AuthService: onAccessOf rejects an ip address mismatch")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  auto mismatch = fixture.authService->onAccessOf(UserTokenLogin{ "192.168.1.2", token->m_id });
  REQUIRE_FALSE(mismatch.has_value());
}

TEST_CASE("AuthService: onLogoutOf throws for an unknown token")
{
  AuthFixture fixture;

  REQUIRE_THROWS_AS(
    fixture.authService->onLogoutOf(UserTokenLogin{ "192.168.1.1", "does-not-exist" }), std::runtime_error);
}

TEST_CASE("AuthService: onLogoutOf keeps the session on an ip address mismatch")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  fixture.authService->onLogoutOf(UserTokenLogin{ "192.168.1.2", token->m_id });
  REQUIRE(fixture.authService->allTokensOf("alice").size() == 1);
}

TEST_CASE("AuthService: onLogoutOf removes a session that arrives in another spelling")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  // a forwarded header carries the same address upper case or compressed; the
  // session must not outlive a logout just because the text differs
  auto token = fixture.authService->onLoginOf(UserLogin{ "2001:db8::1", "alice", "secret1" });
  REQUIRE(token.has_value());

  fixture.authService->onLogoutOf(UserTokenLogin{ "2001:DB8::1", token->m_id });
  REQUIRE(fixture.authService->allTokensOf("alice").empty());
}

TEST_CASE("AuthService: one rate limit budget per canonical client address")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  // the default window allows 20 attempts per ip; alternating between the
  // spellings of one address must not buy a second budget
  for (int attempt = 0; attempt < 10; ++attempt) {
    REQUIRE(fixture.authService->onLoginOf(UserLogin{ "2001:db8::1", "alice", "secret1" }).has_value());
    REQUIRE(fixture.authService->onLoginOf(UserLogin{ "2001:DB8::1", "alice", "secret1" }).has_value());
  }
  auto blocked = fixture.authService->onLoginOf(UserLogin{ "2001:db8::1", "alice", "secret1" });
  REQUIRE_FALSE(blocked.has_value());
}

TEST_CASE("AuthService: a different canonical address keeps its own budget")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  // 192.0.2.1 and 2001:db8::1 are different clients and stay independent
  for (int attempt = 0; attempt < 19; ++attempt) {
    REQUIRE(fixture.authService->onLoginOf(UserLogin{ "192.0.2.1", "alice", "secret1" }).has_value());
  }
  REQUIRE(fixture.authService->onLoginOf(UserLogin{ "192.0.2.1", "alice", "secret1" }).has_value());
  REQUIRE_FALSE(fixture.authService->onLoginOf(UserLogin{ "192.0.2.1", "alice", "secret1" }).has_value());

  auto other = fixture.authService->onLoginOf(UserLogin{ "2001:db8::1", "alice", "secret1" });
  REQUIRE(other.has_value());
}

TEST_CASE("AuthService: onLoginOf upgrades a legacy password hash")
{
  AuthFixture fixture;
  fixture.createUserWithStoredHash("alice", Cryption::hashOfSha512("secret1"));
  REQUIRE_FALSE(Cryption::isModernHash(fixture.userRepository->of("alice")->getPassword()));

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());

  auto stored = fixture.userRepository->of("alice");
  REQUIRE(stored.has_value());
  REQUIRE(Cryption::isModernHash(stored->getPassword()));
}

TEST_CASE("AuthService: onInitialize runs the periodic token check on the scheduler")
{
  AuthFixture fixture;
  fixture.createUser("alice", "secret1");

  auto token = fixture.authService->onLoginOf(UserLogin{ "192.168.1.1", "alice", "secret1" });
  REQUIRE(token.has_value());
  // a failed login leaves an entry in both prune maps
  auto failed = fixture.authService->onLoginOf(UserLogin{ "192.168.1.2", "alice", "wrong" });
  REQUIRE_FALSE(failed.has_value());

  fixture.scheduler->onInitialize();
  fixture.authService->onInitialize();

  // default schedule rate is 2s - give the scheduled onCheck a chance to run
  std::this_thread::sleep_for(std::chrono::milliseconds(2600));

  fixture.authService->onShutdown();
  fixture.scheduler->onShutdown();

  // the still-valid session survives the check
  REQUIRE(fixture.authService->allTokensOf("alice").size() == 1);
}
}// namespace lintel
