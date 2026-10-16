#include <catch2/catch_all.hpp>

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/controller/GroupDto.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/base/controller/UserLoginDto.h"
#include "lintel/features/base/controller/UserPasswordChangeDto.h"
#include "lintel/features/base/controller/UserSessionDto.h"
#include "lintel/features/base/controller/UserSessionsDto.h"
#include "lintel/features/base/controller/UserTokenDto.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/controllers/UserApi.h"
#include "lintel/features/http/provider/ClientProvider.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

namespace lintel {
namespace {

  /** Loopback HTTP backend recording requests and answering with a canned response. */
  class userApiBackend
  {
  public:
    userApiBackend()
    {
      m_server.set_pre_routing_handler([this](const httplib::Request &request, httplib::Response &response) {
        paths.push_back(request.path);
        methods.emplace_back(request.method);
        bodies.emplace_back();
        if (request.method == "POST" || request.method == "PUT" || request.method == "DELETE") {
          return httplib::Server::HandlerResponse::Unhandled;
        }
        response.status = m_responseStatus;
        response.set_content(m_responseBody, "application/json");
        return httplib::Server::HandlerResponse::Handled;
      });
      const auto answer = [this](const httplib::Request &request, httplib::Response &response) {
        if (!bodies.empty()) { bodies.back() = request.body; }
        response.status = m_responseStatus;
        response.set_content(m_responseBody, "application/json");
      };
      m_server.Post(R"(.*)", answer);
      m_server.Put(R"(.*)", answer);
      m_server.Delete(R"(.*)", answer);
      m_port = m_server.bind_to_any_port("127.0.0.1");
      m_thread = std::thread([this]() { m_server.listen_after_bind(); });
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (!m_server.is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    ~userApiBackend()
    {
      m_server.stop();
      if (m_thread.joinable()) { m_thread.join(); }
    }

    userApiBackend(const userApiBackend &) = delete;
    userApiBackend &operator=(const userApiBackend &) = delete;

    void answerWith(const std::string &body)
    {
      m_responseBody = body;
      m_responseStatus = 200;
    }

    void answerWithStatus(int status, const std::string &body)
    {
      m_responseBody = body;
      m_responseStatus = status;
    }

    [[nodiscard]] int port() const { return m_port; }

    std::vector<std::string> paths;
    std::vector<std::string> methods;
    std::vector<std::string> bodies;

  private:
    httplib::Server m_server;
    std::thread m_thread;
    int m_port{ -1 };
    std::string m_responseBody{ "[]" };
    int m_responseStatus{ 200 };
  };

  std::shared_ptr<ClientProvider> userApiProviderOf(int port)
  {
    static const std::string kUserApiAbsentConfigDir = "/nonexistent/lintel-user-api-test-config";
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(EnvironmentConfiguration::ConfigDirectory, kUserApiAbsentConfigDir);
    auto clientConfiguration = std::make_shared<ClientConfiguration>("127.0.0.1",
      port,
      std::chrono::milliseconds(2000),
      std::chrono::milliseconds(2000),
      std::chrono::milliseconds(2000));
    std::vector<std::shared_ptr<Entry>> entries{ std::make_shared<HttpEntry>(
      type_name<HttpComponent>(), clientConfiguration) };
    auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, environment);
    configuration->setEntries(entries);
    return std::make_shared<ClientProvider>(configuration);
  }

  User userApiUser(const std::string &userName = "ada")
  {
    return User("Ada", "Admin", User::Female, "ada@example.test", userName, "storedHash", std::vector<Group>{});
  }

  std::string userApiLoginBody(const std::string &id)
  {
    UserDto dto(userApiUser(), id);
    return dto.JsonSerializable::serialize();
  }

  std::string userApiUsersBody()
  {
    UsersDto dto(std::vector<User>{ userApiUser("ada"), userApiUser("bob") });
    return dto.JsonSerializable::serialize();
  }

  std::string userApiGroupsBody()
  {
    GroupsDto dto(std::vector<Group>{ Group("admin", std::vector<Group>{}, false) });
    return dto.JsonSerializable::serialize();
  }

  std::string userApiSessionsBody()
  {
    UserToken token{ "127.0.0.1",
      "token-1",
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()),
      userApiUser() };
    UserSessionsDto dto(std::vector<UserSessionDto>{ UserSessionDto(token) });
    return dto.JsonSerializable::serialize();
  }

}// namespace

TEST_CASE("UserApi: loginOf returns the user on success")
{
  userApiBackend backend;
  backend.answerWith(userApiLoginBody("user-id"));
  UserApi api(userApiProviderOf(backend.port()));

  UserLoginDto login("ada", "secret");
  const auto result = api.loginOf(login);

  REQUIRE(result.has_value());
  REQUIRE(result->getId() == "user-id");
  REQUIRE(result->getUserName() == "ada");
  REQUIRE(backend.paths.back() == "/user/login");
  REQUIRE(backend.methods.back() == "POST");
}

TEST_CASE("UserApi: loginOf returns nullopt on a non-OK status")
{
  userApiBackend backend;
  backend.answerWithStatus(500, "{}");
  UserApi api(userApiProviderOf(backend.port()));

  REQUIRE_FALSE(api.loginOf(UserLoginDto("ada", "secret")).has_value());
}

TEST_CASE("UserApi: loginOf returns nullopt when no server answers")
{
  UserApi api(userApiProviderOf(1));
  REQUIRE_FALSE(api.loginOf(UserLoginDto("ada", "secret")).has_value());
}

TEST_CASE("UserApi: logoutOf reports the HTTP outcome")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(200, "{}");
  REQUIRE(api.logoutOf(UserTokenDto("token-1")));

  backend.answerWithStatus(401, "{}");
  REQUIRE_FALSE(api.logoutOf(UserTokenDto("token-1")));
}

TEST_CASE("UserApi: changePasswordOf covers OK, unauthorized and error")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));
  const UserPasswordChangeDto dto("ada", "old", "new");

  backend.answerWithStatus(200, "{}");
  REQUIRE(api.changePasswordOf(dto));

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.changePasswordOf(dto), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  REQUIRE_FALSE(api.changePasswordOf(dto));
}

TEST_CASE("UserApi: sessionsOf parses the returned sessions")
{
  userApiBackend backend;
  backend.answerWith(userApiSessionsBody());
  UserApi api(userApiProviderOf(backend.port()));

  const auto sessions = api.sessionsOf();
  REQUIRE(sessions.size() == 1);
  REQUIRE(sessions[0].getId() == "token-1");
  REQUIRE(sessions[0].getUserName() == "ada");
}

TEST_CASE("UserApi: sessionsOf handles unauthorized, server error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.sessionsOf(), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.sessionsOf().empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.sessionsOf().empty());
}

TEST_CASE("UserApi: revokeSessionOf covers OK, unauthorized and error")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(200, "{}");
  REQUIRE(api.revokeSessionOf("token-1"));

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.revokeSessionOf("token-1"), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  REQUIRE_FALSE(api.revokeSessionOf("token-1"));
}

TEST_CASE("UserApi: allOf() returns the groups on success")
{
  userApiBackend backend;
  backend.answerWith(userApiGroupsBody());
  UserApi api(userApiProviderOf(backend.port()));

  const auto groups = api.allOf();
  REQUIRE(groups.size() == 1);
  REQUIRE(groups[0].getGroupName() == "admin");
  REQUIRE(backend.paths.back() == "/user/groups");
}

TEST_CASE("UserApi: allOf() handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allOf(), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allOf().empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allOf().empty());
}

TEST_CASE("UserApi: allUsersOf() returns the users on success")
{
  userApiBackend backend;
  backend.answerWith(userApiUsersBody());
  UserApi api(userApiProviderOf(backend.port()));

  const auto users = api.allUsersOf();
  REQUIRE(users.size() == 2);
  REQUIRE(users[0].getUserName() == "ada");
  REQUIRE(users[1].getUserName() == "bob");
  REQUIRE(backend.paths.back() == "/user/users");
}

TEST_CASE("UserApi: allUsersOf() handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allUsersOf(), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allUsersOf().empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allUsersOf().empty());
}

TEST_CASE("UserApi: allUsersOf(name) returns the matching users")
{
  userApiBackend backend;
  backend.answerWith(userApiUsersBody());
  UserApi api(userApiProviderOf(backend.port()));

  REQUIRE(api.allUsersOf("ada").size() == 2);
  REQUIRE(backend.paths.back() == "/user/users/ada");
}

TEST_CASE("UserApi: allUsersOf(name) handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allUsersOf("ada"), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allUsersOf("ada").empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allUsersOf("ada").empty());
}

TEST_CASE("UserApi: allOf(name) returns the matching groups")
{
  userApiBackend backend;
  backend.answerWith(userApiGroupsBody());
  UserApi api(userApiProviderOf(backend.port()));

  REQUIRE(api.allOf(std::string("admin")).size() == 1);
  REQUIRE(backend.paths.back() == "/user/groups/admin");
}

TEST_CASE("UserApi: allOf(name) handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allOf(std::string("admin")), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allOf(std::string("admin")).empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allOf(std::string("admin")).empty());
}

TEST_CASE("UserApi: allOf(name, virtual) returns the matching groups")
{
  userApiBackend backend;
  backend.answerWith(userApiGroupsBody());
  UserApi api(userApiProviderOf(backend.port()));

  REQUIRE(api.allOf("admin", true).size() == 1);
  REQUIRE(backend.paths.back() == "/user/groups/admin/true");
}

TEST_CASE("UserApi: allOf(name, virtual) handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allOf("admin", false), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allOf("admin", false).empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allOf("admin", false).empty());
}

TEST_CASE("UserApi: allOf(virtual) returns the matching groups")
{
  userApiBackend backend;
  backend.answerWith(userApiGroupsBody());
  UserApi api(userApiProviderOf(backend.port()));

  REQUIRE(api.allOf(false).size() == 1);
  REQUIRE(backend.paths.back() == "/user/groups/false");
}

TEST_CASE("UserApi: allOf(virtual) handles unauthorized, error and bad body")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allOf(true), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allOf(true).empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allOf(true).empty());
}

TEST_CASE("UserApi: createOf covers OK, unauthorized and error")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));
  UserDto dto(userApiUser(), "user-id");

  backend.answerWithStatus(200, "{}");
  api.createOf(dto);
  REQUIRE(backend.paths.back() == "/user/add");

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.createOf(dto), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  api.createOf(dto);
}

TEST_CASE("UserApi: updateOf covers OK, unauthorized and error")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(200, "{}");
  api.updateOf("ada", std::set<std::string>{ "admin" }, std::set<std::string>{ "guest" });
  REQUIRE(backend.paths.back() == "/user/update");

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.updateOf("ada", {}, {}), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  api.updateOf("ada", {}, {});
}

TEST_CASE("UserApi: isLoggedIn reflects the state endpoint")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(200, "{}");
  REQUIRE(api.isLoggedIn());

  backend.answerWithStatus(401, "{}");
  REQUIRE_FALSE(api.isLoggedIn());
}

TEST_CASE("UserApi: deleteOf covers OK, unauthorized and error")
{
  userApiBackend backend;
  UserApi api(userApiProviderOf(backend.port()));

  backend.answerWithStatus(200, "{}");
  api.deleteOf({ "ada", "bob" });
  REQUIRE(backend.paths.back() == "/user/delete");

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.deleteOf({ "ada" }), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  api.deleteOf({ "ada" });
}

}// namespace lintel
