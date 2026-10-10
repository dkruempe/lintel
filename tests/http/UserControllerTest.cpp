#include <httplib.h>
#include <rapidjson/document.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/utils/Cryption.h>
#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/DatabaseConnectionComponent.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/controller/GroupDto.h>
#include <lintel/features/base/controller/UserDto.h>
#include <lintel/features/base/controller/UserGroupDto.h>
#include <lintel/features/base/controller/UserNameDto.h>
#include <lintel/features/base/controller/UserPasswordChangeDto.h>
#include <lintel/features/base/controller/UserSessionsDto.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/Page.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/repositories/GroupRepository.h>
#include <lintel/features/base/repositories/UserRepository.h>
#include <lintel/features/http/controllers/UserController.h>
#include <lintel/features/http/service/ContentType.h>
#include <lintel/features/http/service/HttpStatusCodes.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockAuthService.h"

using namespace trompeloeil;

namespace {

// `uc` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `adminGroup` or `fixture`
// would collide (and shadow local variables, Clang -Wshadow).
constexpr std::chrono::milliseconds ucStartupTimeout{ 5000 };
constexpr char ucTokenAdmin[] = "user-token-admin";
constexpr char ucTokenPlain[] = "user-token-plain";
constexpr char ucTokenOutsider[] = "user-token-outsider";
constexpr char ucSessionOwn[] = "user-session-own";
constexpr char ucSessionForeign[] = "user-session-foreign";

Group ucAdminGroup{ "Admin-User", {}, true };
Group ucUserGroup{ "User-User", {}, true };
Group ucUnrelatedGroup{ "Some-Other-Group", {}, true };

// the two roles the controller distinguishes, plus a group-less bystander
User ucAdminUser("Ada", "Admin", User::Female, "admin@uc.test", "ucAdmin", "storedHash", { ucAdminGroup });
User ucPlainUser("Peri", "Plain", User::Male, "plain@uc.test", "ucPlain", "storedHash", { ucUserGroup });
User ucOutsiderUser("Otto", "Other", User::Male, "out@uc.test", "ucOutsider", "storedHash", { ucUnrelatedGroup });

UserToken ucTokenOf(const User &user, const std::string &id)
{
  return UserToken{
    "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
  };
}

/** @return the Authorization header that carries the given bearer token */
httplib::Headers ucAuthHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
}

/** @return the Authorization header for HTTP Basic auth */
httplib::Headers ucBasicHeaders(const std::string &userName, const std::string &password)
{
  return httplib::Headers{ { "Authorization", "Basic " + Cryption::encodeBase64(userName + ":" + password) } };
}

std::string ucNowSerialized()
{
  return StringifyService<date::sys_time<std::chrono::microseconds>>::serializeToString(
    std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()));
}

/**
 * The ADD_HANDLER_METHODs of UserController live in the private section, so the
 * only reachable entry point is Controller::registerMethods(). The controller is
 * therefore driven through a real httplib server, exactly as in production.
 *
 * UserController takes the concrete GroupRepository and UserRepository, so the
 * data paths need a real SQLite database behind them; the auth service stays a
 * mock because it owns token state that would otherwise leak between tests.
 */
struct UserControllerFixture
{
  // must stay the first member: it has to be set before the EnvironmentConfiguration below reads it
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_user_controller_test_cfg" };
  std::string m_dbPath;
  std::shared_ptr<EnvironmentConfiguration> m_envConfig;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<GroupRepository> m_groupRepository;
  std::shared_ptr<UserRepository> m_userRepository;
  std::shared_ptr<MockAuthService> m_authService;
  std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
  std::thread m_serverThread;
  int m_port = -1;
  std::unique_ptr<httplib::Client> m_client;

  UserControllerFixture()
  {
    static int counter = 0;
    m_dbPath = (std::filesystem::temp_directory_path()
                / ("user_controller_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + ".db"))
                 .string();
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

    m_envConfig = std::make_shared<EnvironmentConfiguration>();
    m_configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, m_envConfig);
    auto entry = std::make_shared<DatabaseConnectionEntry>(
      type_name<DatabaseConnectionComponent>(), m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
    m_configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ entry });
    m_connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(m_configuration);
    m_groupRepository = std::make_shared<GroupRepository>(m_connectionConfigurations);
    m_userRepository = std::make_shared<UserRepository>(m_connectionConfigurations, m_groupRepository);
    m_authService = std::make_shared<MockAuthService>();

    // Seed the groups directly and load the cache exactly once: GroupRepository::createOf() inserts
    // into the cache itself, so a following onAwake() would double every group in allOf().
    {
      db::Connection seedConnection(db::ConnectionType::SQLite, m_dbPath);
      db::Statement seedStatement(seedConnection);
      auto connectionEntry = m_connectionConfigurations->ofDefault();
      seedStatement.execute("insert into groups(name, virtual) values(?, ?)",
        db::ParameterBuilder(connectionEntry).add(std::string("ucAdmins")).add(false));
      seedStatement.execute("insert into groups(name, virtual) values(?, ?)",
        db::ParameterBuilder(connectionEntry).add(std::string("ucUsers")).add(false));
    }
    m_groupRepository->onAwake();
  }

  ~UserControllerFixture()
  {
    if (m_server != nullptr) { m_server->stop(); }
    if (m_serverThread.joinable()) { m_serverThread.join(); }
    std::remove(m_dbPath.c_str());
  }

  UserControllerFixture(const UserControllerFixture &) = delete;
  UserControllerFixture &operator=(const UserControllerFixture &) = delete;

  /** Create a persisted user with the given password hash */
  void
    seedUser(const std::string &userName, const std::string &hash = "ucSeedHash", const std::vector<Group> &groups = {})
  {
    m_userRepository->createOf(User("Seeded", "User", User::Male, userName + "@uc.test", userName, hash, groups));
  }

  UserController makeController() const { return UserController(m_authService, m_groupRepository, m_userRepository); }

  /** Register the routes of the controller, bind an ephemeral port and serve */
  void startServing(Controller &controller)
  {
    controller.registerMethods(m_server);
    m_port = m_server->bind_to_any_port("127.0.0.1");
    if (m_port <= 0) { return; }
    m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
    const auto deadline = std::chrono::steady_clock::now() + ucStartupTimeout;
    while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!m_server->is_running()) { return; }
    m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
    m_client->set_connection_timeout(std::chrono::seconds(10));
    m_client->set_read_timeout(std::chrono::seconds(10));
  }

  bool isReady() const { return m_client != nullptr && m_server->is_running(); }

  /** @return the group array produced by the controller, parsed back */
  std::vector<GroupDto> parseGroups(const std::string &json) const
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    GroupsDto parsed;
    parsed.deserialize(document);
    return parsed.getGroups();
  }

  /** @return the user array produced by the controller, parsed back */
  std::vector<UserDto> parseUsers(const std::string &json) const
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    UsersDto parsed;
    parsed.deserialize(document);
    return parsed.getUsers();
  }

  /** @return the paging envelope produced by the controller, parsed back */
  UsersPageDto parseUserEnvelope(const std::string &json) const
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    UsersPageDto parsed;
    parsed.deserialize(document);
    return parsed;
  }
};

/** @return the request body of POST /user/add */
std::string ucAddUserBody(const std::string &userName, const std::string &plainPassword)
{
  return std::string{ R"({"first_name":"New","last_name":"User","email":")" } + userName + "@uc.test"
         + R"(","user_name":")" + userName + R"(","sex":"male","password":")" + Cryption::encodeBase64(plainPassword)
         + R"(","created_timestamp":")" + ucNowSerialized() + R"("})";
}

}// namespace

// --------------------------------------------------------------------------------------
// /user/login
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::loginOfPost returns the UserDto and the token id of a successful login", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));
  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto response = fixture.m_client->Post("/user/login", ucBasicHeaders("ucAdmin", "secret"), "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  rapidjson::Document document;
  document.Parse(response->body.c_str());
  UserDto parsed;
  REQUIRE(parsed.deserialize(document));
  REQUIRE(parsed.getUserName() == "ucAdmin");
  REQUIRE(parsed.getId() == ucTokenAdmin);
  REQUIRE(parsed.getFirstName() == "Ada");
}

TEST_CASE("UserController::loginOfPost forwards the decoded basic credentials", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  UserLogin captured{};
  REQUIRE_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)))
    .LR_SIDE_EFFECT(captured = _1)
    .TIMES(1)
    .LR_RETURN(std::optional<UserToken>{});

  auto response = fixture.m_client->Post("/user/login", ucBasicHeaders("ucAdmin", "s3cret"), "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(captured.m_userName == "ucAdmin");
  REQUIRE(captured.m_password == "s3cret");
  REQUIRE(captured.m_ipAddress == "127.0.0.1");
}

TEST_CASE("UserController::loginOfPost returns 403 for wrong credentials", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)))
    .TIMES(1)
    .LR_RETURN(std::optional<UserToken>{});

  auto response = fixture.m_client->Post("/user/login", ucBasicHeaders("ucAdmin", "wrong"), "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::loginOfPost returns 403 for a malformed basic header", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // base64 of "no-colon-here" - decoded there is no ':' to split user and password
  const httplib::Headers headers{ { "Authorization", "Basic " + Cryption::encodeBase64("no-colon-here") } };
  FORBID_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)));

  auto response = fixture.m_client->Post("/user/login", headers, "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::loginOfPost returns 403 without an authorization header", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)));

  auto response = fixture.m_client->Post("/user/login", "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::loginOfPost returns 403 when the caller is already logged in", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));
  FORBID_CALL(*fixture.m_authService, onLoginOf(ANY(const UserLogin &)));

  auto response = fixture.m_client->Post("/user/login", ucAuthHeaders(ucTokenAdmin), "", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

// --------------------------------------------------------------------------------------
// /user/logout and /user/state
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::logoutOfDelete returns 401 without a bearer token", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onLogoutOf(ANY(const UserTokenLogin &)));

  auto response = fixture.m_client->Delete("/user/logout");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::logoutOfDelete invalidates the token of the caller", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));
  REQUIRE_CALL(*fixture.m_authService, onLogoutOf(ANY(const UserTokenLogin &))).TIMES(1);

  auto response = fixture.m_client->Delete("/user/logout", ucAuthHeaders(ucTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::loginStateOfGet returns 200 with an empty body for a valid token", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));

  auto response = fixture.m_client->Get("/user/state", ucAuthHeaders(ucTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::loginStateOfGet returns 401 without a bearer token", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto response = fixture.m_client->Get("/user/state");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

// --------------------------------------------------------------------------------------
// /user/groups
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::allGroupsOfGet returns 401 for a user outside Admin-User and User-User", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucOutsiderUser, ucTokenOutsider));

  auto response = fixture.m_client->Get("/user/groups", ucAuthHeaders(ucTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::allGroupsOfGet returns 403 for a non json content type", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto headers = ucAuthHeaders(ucTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Get("/user/groups", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::allGroupsOfGet lists all groups of the group repository", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/groups", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto groups = fixture.parseGroups(response->body);
  REQUIRE(groups.size() == 2);
  REQUIRE(groups[0].getGroupName() == "ucAdmins");
  REQUIRE(groups[1].getGroupName() == "ucUsers");
}

TEST_CASE("UserController::allGroupsOfGroupNameOrIsVirtualGroupGet maps 'true' to the virtual filter",
  "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/groups/true", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  // the two seeded groups are real, so the virtual filter matches nothing
  REQUIRE(response->body == "[]");
  REQUIRE(fixture.parseGroups(response->body).empty());
}

TEST_CASE("UserController::allGroupsOfGroupNameOrIsVirtualGroupGet treats other values as a regex", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/groups/ucAdmin.*", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto groups = fixture.parseGroups(response->body);
  REQUIRE(groups.size() == 1);
  REQUIRE(groups[0].getGroupName() == "ucAdmins");
}

TEST_CASE("UserController::allGroupsOfGroupNameAndIsVirtualGroupGet combines name and virtual filter",
  "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(2)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto matching = fixture.m_client->Get("/user/groups/ucUsers/false", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(matching != nullptr);
  REQUIRE(matching->status == 200);
  const auto matchedGroups = fixture.parseGroups(matching->body);
  REQUIRE(matchedGroups.size() == 1);
  REQUIRE(matchedGroups[0].getGroupName() == "ucUsers");

  auto notMatching = fixture.m_client->Get("/user/groups/ucUsers/true", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(notMatching != nullptr);
  REQUIRE(notMatching->status == 200);
  REQUIRE(notMatching->body == "[]");
}

// --------------------------------------------------------------------------------------
// /user/users
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::allUsersOfGet lists every user", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucSeedOne");
  fixture.seedUser("ucSeedTwo");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/users", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto users = fixture.parseUsers(response->body);
  REQUIRE(users.size() == 2);
  REQUIRE(users[0].getUserName() == "ucSeedOne");
  REQUIRE(users[1].getUserName() == "ucSeedTwo");
}

TEST_CASE("UserController::allUsersOfGet returns the paging envelope when a limit is given", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucSeedOne");
  fixture.seedUser("ucSeedTwo");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/users?after=ucSeed&limit=1", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto envelope = fixture.parseUserEnvelope(response->body);
  REQUIRE(envelope.getUsers().size() == 1);
  REQUIRE(envelope.getUsers()[0].getUserName() == "ucSeedOne");
  REQUIRE(envelope.hasMore());
  REQUIRE(envelope.getNextAfter().has_value());
  REQUIRE(envelope.getNextAfter().value() == "ucSeedOne");
}

TEST_CASE("UserController::allUsersOfGet returns 400 for a limit above the hard cap", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/users?limit=1001", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 400);
  REQUIRE(response->body == "bad request");
}

TEST_CASE("UserController::allUsersOfUserNameGet filters by a user name regex", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucAdminOne");
  fixture.seedUser("ucAdminTwo");
  fixture.seedUser("ucPlainOne");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Get("/user/users/ucAdmin.*", ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto users = fixture.parseUsers(response->body);
  REQUIRE(users.size() == 2);
  REQUIRE(users[0].getUserName() == "ucAdminOne");
  REQUIRE(users[1].getUserName() == "ucAdminTwo");
}

// --------------------------------------------------------------------------------------
// /user/add
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::addUserPost returns 401 for a user without Admin-User", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));

  auto response = fixture.m_client->Post(
    "/user/add", ucAuthHeaders(ucTokenPlain), ucAddUserBody("ucNewbie", "secret"), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE_FALSE(fixture.m_userRepository->of("ucNewbie").has_value());
}

TEST_CASE("UserController::addUserPost returns 406 when the password is missing", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  const std::string body = R"({"first_name":"New","last_name":"User","email":"n@uc.test",)"
                           R"("user_name":"ucNewbie","sex":"male","created_timestamp":")"
                           + ucNowSerialized() + R"("})";
  auto response = fixture.m_client->Post("/user/add", ucAuthHeaders(ucTokenAdmin), body, "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 406);
  REQUIRE(response->body.empty());
  REQUIRE_FALSE(fixture.m_userRepository->of("ucNewbie").has_value());
}

TEST_CASE("UserController::addUserPost stores the new user with a hashed password", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Post(
    "/user/add", ucAuthHeaders(ucTokenAdmin), ucAddUserBody("ucNewbie", "s3cret"), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto created = fixture.m_userRepository->of("ucNewbie");
  REQUIRE(created.has_value());
  REQUIRE(created->getFirstName() == "New");
  REQUIRE(created->getEmail() == "ucNewbie@uc.test");
  // the plaintext must not be stored, and the hash must verify against it
  REQUIRE(created->getPassword() != "s3cret");
  REQUIRE(Cryption::verifyOf("s3cret", created->getPassword()));
}

// --------------------------------------------------------------------------------------
// /user/update
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::updateUserPut returns 403 for an unknown target user", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  const std::string body = R"({"user_name":"ucGhost","groups_add":["ucAdmins"],"groups_remove":[]})";
  auto response = fixture.m_client->Put("/user/update", ucAuthHeaders(ucTokenAdmin), body, "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::updateUserPut adds and removes group memberships", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucTarget", "hash");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(2)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto added = fixture.m_client->Put("/user/update",
    ucAuthHeaders(ucTokenAdmin),
    R"({"user_name":"ucTarget","groups_add":["ucAdmins"],"groups_remove":[]})",
    "application/json");
  REQUIRE(added != nullptr);
  REQUIRE(added->status == 200);
  auto stored = fixture.m_userRepository->of("ucTarget");
  REQUIRE(stored.has_value());
  REQUIRE(stored->getGroups().size() == 1);
  REQUIRE(stored->getGroups()[0].getGroupName() == "ucAdmins");

  auto removed = fixture.m_client->Put("/user/update",
    ucAuthHeaders(ucTokenAdmin),
    R"({"user_name":"ucTarget","groups_add":[],"groups_remove":["ucAdmins"]})",
    "application/json");
  REQUIRE(removed != nullptr);
  REQUIRE(removed->status == 200);
  auto cleared = fixture.m_userRepository->of("ucTarget");
  REQUIRE(cleared.has_value());
  REQUIRE(cleared->getGroups().empty());
}

// --------------------------------------------------------------------------------------
// /user/delete
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::deleteUser returns 403 when the caller deletes itself", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucVictim", "hash");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Delete(
    "/user/delete", ucAuthHeaders(ucTokenAdmin), R"([{"user_name":"ucAdmin"}])", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::deleteUser removes a listed user", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucVictim", "hash");
  fixture.seedUser("ucSurvivor", "hash");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  auto response = fixture.m_client->Delete(
    "/user/delete", ucAuthHeaders(ucTokenAdmin), R"([{"user_name":"ucVictim"}])", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE_FALSE(fixture.m_userRepository->of("ucVictim").has_value());
  REQUIRE(fixture.m_userRepository->of("ucSurvivor").has_value());
}

// KNOWN DEFECT, deliberately not asserted here: UserRepository::deleteOf(const std::vector<std::string>&)
// calls transaction.commit() *inside* its loop (src/features/base/repositories/UserRepository.cpp:522),
// so the second and every further user name raise "SQLite Transaction was finished before with commit or
// rollback" and DELETE /user/delete answers 500 although swagger.yaml documents 200. A test that pinned the
// current 500 would fail as soon as the repository is fixed, so the defect is reported instead of locked in.

TEST_CASE("UserController::deleteUser returns 401 for a caller without Admin-User", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucVictim", "hash");
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));

  auto response = fixture.m_client->Delete(
    "/user/delete", ucAuthHeaders(ucTokenPlain), R"([{"user_name":"ucVictim"}])", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(fixture.m_userRepository->of("ucVictim").has_value());
}

// --------------------------------------------------------------------------------------
// /user/password
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::changePasswordOfPut returns 403 for an empty user name", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  UserPasswordChangeDto dto("", Cryption::encodeBase64("old"), Cryption::encodeBase64("new"));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenAdmin), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::changePasswordOfPut returns 406 for an empty new password", "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucTarget", Cryption::hashOf("old"));
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  UserPasswordChangeDto dto("ucTarget", Cryption::encodeBase64("old"), Cryption::encodeBase64(""));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenAdmin), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 406);
  REQUIRE(response->body.empty());

  auto unchanged = fixture.m_userRepository->of("ucTarget");
  REQUIRE(unchanged.has_value());
  REQUIRE(Cryption::verifyOf("old", unchanged->getPassword()));
}

TEST_CASE("UserController::changePasswordOfPut returns 401 when a plain user changes a foreign password",
  "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucTarget", Cryption::hashOf("old"));
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));

  UserPasswordChangeDto dto("ucTarget", Cryption::encodeBase64("old"), Cryption::encodeBase64("new"));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenPlain), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());

  auto unchanged = fixture.m_userRepository->of("ucTarget");
  REQUIRE(unchanged.has_value());
  REQUIRE(Cryption::verifyOf("old", unchanged->getPassword()));
}

TEST_CASE("UserController::changePasswordOfPut returns 403 for a wrong old password in self service",
  "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucSelf", Cryption::hashOf("old"));
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // self service: the caller's own account, identified by the same user name
  User selfService("Peri", "Plain", User::Male, "plain@uc.test", "ucSelf", "storedHash", { ucUserGroup });
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(selfService, ucTokenPlain));

  UserPasswordChangeDto dto("ucSelf", Cryption::encodeBase64("wrong"), Cryption::encodeBase64("new"));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenPlain), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());

  auto unchanged = fixture.m_userRepository->of("ucSelf");
  REQUIRE(unchanged.has_value());
  REQUIRE(Cryption::verifyOf("old", unchanged->getPassword()));
}

TEST_CASE(
  "UserController::changePasswordOfPut lets an admin change a foreign password without the "
  "old one",
  "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucTarget", Cryption::hashOf("old"));
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));

  UserPasswordChangeDto dto("ucTarget", "", Cryption::encodeBase64("new"));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenAdmin), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.empty());

  auto changed = fixture.m_userRepository->of("ucTarget");
  REQUIRE(changed.has_value());
  REQUIRE(Cryption::verifyOf("new", changed->getPassword()));
}

TEST_CASE("UserController::changePasswordOfPut completes a self service change with the old password",
  "[user_controller]")
{
  UserControllerFixture fixture;
  fixture.seedUser("ucSelf", Cryption::hashOf("old"));
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  User selfService("Peri", "Plain", User::Male, "plain@uc.test", "ucSelf", "storedHash", { ucUserGroup });
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(selfService, ucTokenPlain));

  UserPasswordChangeDto dto("ucSelf", Cryption::encodeBase64("old"), Cryption::encodeBase64("new"));
  auto response = fixture.m_client->Put(
    "/user/password", ucAuthHeaders(ucTokenPlain), dto.JsonSerializable::serialize(), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  auto changed = fixture.m_userRepository->of("ucSelf");
  REQUIRE(changed.has_value());
  REQUIRE(Cryption::verifyOf("new", changed->getPassword()));
}

// --------------------------------------------------------------------------------------
// /user/sessions
// --------------------------------------------------------------------------------------

TEST_CASE("UserController::allSessionsOfGet lists the sessions of the caller", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));
  REQUIRE_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)))
    .TIMES(1)
    .LR_RETURN(
      std::vector<UserToken>{ ucTokenOf(ucPlainUser, ucSessionOwn), ucTokenOf(ucPlainUser, ucSessionForeign) });

  auto response = fixture.m_client->Get("/user/sessions", ucAuthHeaders(ucTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  UserSessionsDto parsed;
  rapidjson::Document document;
  document.Parse(response->body.c_str());
  REQUIRE(parsed.deserialize(document));
  REQUIRE(parsed.getSessions().size() == 2);
  REQUIRE(parsed.getSessions()[0].getId() == ucSessionOwn);
  REQUIRE(parsed.getSessions()[1].getId() == ucSessionForeign);
  REQUIRE(parsed.getSessions()[0].getUserName() == "ucPlain");
}

TEST_CASE("UserController::allSessionsOfGet returns 401 for a user outside Admin-User and User-User",
  "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucOutsiderUser, ucTokenOutsider));
  FORBID_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)));

  auto response = fixture.m_client->Get("/user/sessions", ucAuthHeaders(ucTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::revokeSessionOf returns 401 when a plain user revokes a foreign session",
  "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));
  REQUIRE_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)))
    .TIMES(1)
    .LR_RETURN(std::vector<UserToken>{ ucTokenOf(ucPlainUser, ucSessionOwn) });
  FORBID_CALL(*fixture.m_authService, revokeTokenOf(ANY(const std::string &)));

  auto response =
    fixture.m_client->Delete("/user/sessions/" + std::string(ucSessionForeign), ucAuthHeaders(ucTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::revokeSessionOf lets a plain user revoke its own session", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucPlainUser, ucTokenPlain));
  REQUIRE_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)))
    .TIMES(1)
    .LR_RETURN(std::vector<UserToken>{ ucTokenOf(ucPlainUser, ucSessionOwn) });
  REQUIRE_CALL(*fixture.m_authService, revokeTokenOf(ANY(const std::string &))).TIMES(1);

  auto response = fixture.m_client->Delete("/user/sessions/" + std::string(ucSessionOwn), ucAuthHeaders(ucTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.empty());
}

TEST_CASE("UserController::revokeSessionOf lets an admin revoke a foreign session", "[user_controller]")
{
  UserControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(ucTokenOf(ucAdminUser, ucTokenAdmin));
  REQUIRE_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)))
    .TIMES(1)
    .LR_RETURN(std::vector<UserToken>{});
  REQUIRE_CALL(*fixture.m_authService, revokeTokenOf(ANY(const std::string &))).TIMES(1);

  auto response =
    fixture.m_client->Delete("/user/sessions/" + std::string(ucSessionForeign), ucAuthHeaders(ucTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.empty());
}
