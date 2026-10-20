#include <httplib.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <lintel/core/services/LoggerService.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/services/IAuthService.h>
#include <lintel/features/http/controllers/AdminController.h>
#include <lintel/features/http/service/ContentType.h>
#include <lintel/features/http/service/TlsReloadRegistry.h>

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "../mocks/MockAuthService.h"

namespace lintel {
using namespace trompeloeil;

namespace {

  // `ac` prefix: unity builds merge several test files into one translation unit
  // and share one anonymous namespace, so a generic `adminGroup` or `fixture`
  // would collide (and shadow local variables, Clang -Wshadow).
  constexpr std::chrono::milliseconds acStartupTimeout{ 5000 };
  constexpr char acTokenAdmin[] = "admin-token-admin";
  constexpr char acTokenLogAdmin[] = "admin-token-log-admin";
  constexpr char acTokenOutsider[] = "admin-token-outsider";

  Group acAdminGroup{ "Admin-Tls", {}, true };
  Group acLogGroup{ "Admin-Log", {}, true };
  Group acUnrelatedGroup{ "Some-Other-Group", {}, true };

  User acAdmin("Admin", "Tls", User::Male, "admin-tls@test.com", "tlsAdmin", "", { acAdminGroup });
  User acLogAdmin("Admin", "Log", User::Male, "admin-log@test.com", "logAdmin", "", { acLogGroup });
  User acOutsider("Admin", "Outsider", User::Male, "admin-out@test.com", "tlsOut", "", { acUnrelatedGroup });

  UserToken acTokenOf(const User &user, const std::string &id)
  {
    return UserToken{
      "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
    };
  }

  httplib::Headers acAuthHeaders(const char *token)
  {
    return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
  }

  /** The reload handlers live in TlsReloadRegistry, so the controller is driven
   *  through a real httplib server, exactly as in production. */
  struct AdminControllerFixture
  {
    std::shared_ptr<MockAuthService> m_authService;
    std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
    std::thread m_serverThread;
    int m_port = -1;
    std::unique_ptr<httplib::Client> m_client;

    AdminControllerFixture() { m_authService = std::make_shared<MockAuthService>(); }

    ~AdminControllerFixture()
    {
      if (m_server != nullptr) { m_server->stop(); }
      if (m_serverThread.joinable()) { m_serverThread.join(); }
      // the registry is process wide; leave it empty for the other cases
      TlsReloadRegistry::instance().clear();
    }

    AdminControllerFixture(const AdminControllerFixture &) = delete;
    AdminControllerFixture &operator=(const AdminControllerFixture &) = delete;

    /** Register the routes of the controller, bind an ephemeral port and serve */
    void startServing(Controller &controller)
    {
      controller.registerMethods(m_server);
      m_port = m_server->bind_to_any_port("127.0.0.1");
      if (m_port <= 0) { return; }
      m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
      const auto deadline = std::chrono::steady_clock::now() + acStartupTimeout;
      while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (!m_server->is_running()) { return; }
      m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
      m_client->set_connection_timeout(std::chrono::seconds(5));
      m_client->set_read_timeout(std::chrono::seconds(5));
    }

    bool isReady() const { return m_client != nullptr && m_server->is_running(); }

    /** @return the response body parsed as json, failing the test on invalid json */
    static rapidjson::Document acParse(const std::string &json)
    {
      rapidjson::Document document;
      document.Parse(json.c_str());
      REQUIRE_FALSE(document.HasParseError());
      return document;
    }
  };

}// namespace

TEST_CASE("AdminController::reloadTlsOfPost returns 401 without a bearer token", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // without a bearer token the wrapper never asks the auth service, and the
  // registry stays untouched
  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto response = fixture.m_client->Post("/admin/reload-tls");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("AdminController::reloadTlsOfPost returns 401 for a user outside Admin-Tls", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acOutsider, acTokenOutsider));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("AdminController::reloadTlsOfPost returns 403 for a non json content type", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto headers = acAuthHeaders(acTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Post("/admin/reload-tls", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("AdminController::reloadTlsOfPost returns 409 when nothing is registered", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  TlsReloadRegistry::instance().clear();

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 409);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(body.HasMember("status"));
  REQUIRE(body["status"].IsString());
  REQUIRE(std::string(body["status"].GetString()) == "nothing_to_reload");
  REQUIRE(body.HasMember("results"));
  REQUIRE(body["results"].IsObject());
  REQUIRE(body["results"].MemberCount() == 0);
}

TEST_CASE("AdminController::reloadTlsOfPost reports ok for a successful handler", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  int calls = 0;
  TlsReloadRegistry::instance().registerHandler("tls", [&calls]() {
    ++calls;
    return std::string();
  });

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(calls == 1);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(std::string(body["status"].GetString()) == "reloaded");
  REQUIRE(body["results"].HasMember("tls"));
  REQUIRE(std::string(body["results"]["tls"].GetString()) == "ok");
}

TEST_CASE("AdminController::reloadTlsOfPost returns 500 and keeps the reason", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  TlsReloadRegistry::instance().registerHandler(
    "tls", []() { return std::string("cannot read the certificate file '/etc/lintel/tls/server.crt'"); });

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 500);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(std::string(body["status"].GetString()) == "failed");
  REQUIRE(std::string(body["results"]["tls"].GetString())
          == "failed: cannot read the certificate file '/etc/lintel/tls/server.crt'");
}

TEST_CASE("AdminController::reloadTlsOfPost reports a throwing handler as failed", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  TlsReloadRegistry::instance().registerHandler("tls", []() -> std::string { throw std::runtime_error("boom"); });

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 500);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(std::string(body["results"]["tls"].GetString()) == "failed: boom");
}

TEST_CASE("AdminController::reloadTlsOfPost aggregates every registered handler", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  TlsReloadRegistry::instance().registerHandler("tls", []() { return std::string(); });
  TlsReloadRegistry::instance().registerHandler("demo", []() { return std::string("rejected"); });

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 500);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(body["results"].MemberCount() == 2);
  REQUIRE(std::string(body["results"]["tls"].GetString()) == "ok");
  REQUIRE(std::string(body["results"]["demo"].GetString()) == "failed: rejected");
}

TEST_CASE("AdminController::reloadTlsOfPost escapes a failure reason", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // the reason comes from an exception message and may contain anything; it
  // must not turn the response into invalid json
  const std::string reason = "path \"C:\\certs\\server.crt\"\nand a tab\there";
  TlsReloadRegistry::instance().registerHandler("tls", [&reason]() { return reason; });

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acAdmin, acTokenAdmin));

  auto response = fixture.m_client->Post("/admin/reload-tls", acAuthHeaders(acTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 500);

  const auto body = AdminControllerFixture::acParse(response->body);
  const std::string expected = "failed: " + reason;
  REQUIRE(std::string(body["results"]["tls"].GetString()) == expected);
}

/** Authorization plus an application/json body, as the log-level endpoint requires. */
httplib::Headers acLevelJsonHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token },
    { "Content-Type", "application/json" } };
}

TEST_CASE("AdminController::logLevelOfPost changes the level at runtime", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // the level is process wide: put it back so a single-process run of the test
  // binary does not carry debug into the tests that run after this one
  const LogLevel previousLevel = LoggerService::get().levelOf();

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acLogAdmin, acTokenLogAdmin));

  auto response = fixture.m_client->Post(
    "/admin/log-level", acLevelJsonHeaders(acTokenLogAdmin), R"({"level":"debug"})", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(LoggerService::get().levelOf() == LogLevel::debug);

  const auto body = AdminControllerFixture::acParse(response->body);
  REQUIRE(std::string(body["status"].GetString()) == "changed");
  REQUIRE(std::string(body["level"].GetString()) == "debug");

  LoggerService::get().setLevel(previousLevel);
}

TEST_CASE("AdminController::logLevelOfPost answers 401 outside the admin group", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(acTokenOf(acOutsider, acTokenOutsider));

  auto response = fixture.m_client->Post(
    "/admin/log-level", acLevelJsonHeaders(acTokenOutsider), R"({"level":"trace"})", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("AdminController::logLevelOfPost answers 400 for an unknown or mistyped level", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  const LogLevel previousLevel = LoggerService::get().levelOf();

  SECTION("unknown level name")
  {
    REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
      .TIMES(1)
      .LR_RETURN(acTokenOf(acLogAdmin, acTokenLogAdmin));

    auto response = fixture.m_client->Post(
      "/admin/log-level", acLevelJsonHeaders(acTokenLogAdmin), R"({"level":"verbose"})", "application/json");
    REQUIRE(response != nullptr);
    REQUIRE(response->status == 400);

    const auto body = AdminControllerFixture::acParse(response->body);
    REQUIRE(std::string(body["status"].GetString()) == "invalid_level");
    // a rejected level must not have touched the effective one
    REQUIRE(LoggerService::get().levelOf() == previousLevel);
  }

  SECTION("level is not a string")
  {
    REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
      .TIMES(1)
      .LR_RETURN(acTokenOf(acLogAdmin, acTokenLogAdmin));

    auto response = fixture.m_client->Post(
      "/admin/log-level", acLevelJsonHeaders(acTokenLogAdmin), R"({"level":42})", "application/json");
    REQUIRE(response != nullptr);
    REQUIRE(response->status == 400);
    REQUIRE(LoggerService::get().levelOf() == previousLevel);
  }

  SECTION("body is not json")
  {
    REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
      .TIMES(1)
      .LR_RETURN(acTokenOf(acLogAdmin, acTokenLogAdmin));

    auto response = fixture.m_client->Post(
      "/admin/log-level", acLevelJsonHeaders(acTokenLogAdmin), "not json at all", "application/json");
    REQUIRE(response != nullptr);
    REQUIRE(response->status == 400);
  }
}

TEST_CASE("AdminController answers with an X-Request-Id correlation id", "[admin_controller]")
{
  AdminControllerFixture fixture;
  AdminController controller(fixture.m_authService);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // both requests below run through the auth service, hence twice
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(2)
    .LR_RETURN(acTokenOf(acLogAdmin, acTokenLogAdmin));

  // a client supplied correlation id has to survive the round trip, otherwise a
  // caller cannot tie the response back to the request it sent
  auto echoed = fixture.m_client->Post("/admin/log-level",
    httplib::Headers{ { "Authorization", std::string("Bearer ") + acTokenLogAdmin },
      { "Content-Type", "application/json" },
      { "X-Request-Id", "my-correlation-1" } },
    R"({"level":"info"})",
    "application/json");
  REQUIRE(echoed != nullptr);
  REQUIRE(echoed->get_header_value("X-Request-Id") == "my-correlation-1");

  // without an id the server generates one, so every response carries a key to
  // search for in the log of the handling process
  auto generated = fixture.m_client->Post(
    "/admin/log-level", acLevelJsonHeaders(acTokenLogAdmin), R"({"level":"info"})", "application/json");
  REQUIRE(generated != nullptr);
  const std::string generatedId = generated->get_header_value("X-Request-Id");
  REQUIRE(!generatedId.empty());
  REQUIRE(generatedId != "my-correlation-1");
}
}// namespace lintel
