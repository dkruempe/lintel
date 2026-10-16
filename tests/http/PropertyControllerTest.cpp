#include <httplib.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../mocks/MockAuthService.h"
#include "../mocks/MockHistoryService.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/http/controllers/PropertyController.h"
#include "lintel/features/property/services/PropertyService.h"

namespace lintel {
using namespace trompeloeil;

namespace {

  constexpr std::chrono::milliseconds propctlStartupTimeout{ 5000 };

  constexpr char propctlTokenAdmin[] = "prop-token-admin";
  constexpr char propctlTokenUser[] = "prop-token-user";
  constexpr char propctlTokenOutsider[] = "prop-token-outsider";

  Group propctlAdminGroup{ "Admin-Property", {}, true };
  Group propctlUserGroup{ "User-Property", {}, true };
  Group propctlOtherGroup{ "Some-Other-Group", {}, true };

  User propctlAdminUser("Ada", "Admin", User::Female, "admin@pc.test", "pcAdmin", "hash", { propctlAdminGroup });
  User propctlPlainUser("Peri", "Plain", User::Male, "plain@pc.test", "pcPlain", "hash", { propctlUserGroup });
  User propctlOutsiderUser("Otto", "Other", User::Male, "out@pc.test", "pcOutsider", "hash", { propctlOtherGroup });

  UserToken propctlTokenOf(const User &user, const std::string &id)
  {
    return UserToken{
      "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
    };
  }

  httplib::Headers propctlAuthHeaders(const char *token)
  {
    return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
  }

  std::optional<UserToken> propctlResolveToken(const UserTokenLogin &login)
  {
    if (login.m_id == propctlTokenAdmin) { return propctlTokenOf(propctlAdminUser, propctlTokenAdmin); }
    if (login.m_id == propctlTokenUser) { return propctlTokenOf(propctlPlainUser, propctlTokenUser); }
    if (login.m_id == propctlTokenOutsider) { return propctlTokenOf(propctlOutsiderUser, propctlTokenOutsider); }
    return std::nullopt;
  }

  std::shared_ptr<PropertyService> propctlMakeService()
  {
    return std::make_shared<PropertyService>(std::vector<std::shared_ptr<PropertyRepository>>{},
      std::vector<std::shared_ptr<AbstractServiceInterface>>{});
  }

  /** Drives PropertyController through a real httplib server, because its
   *  ADD_HANDLER_METHODs live in the private section and the only public entry
   *  point is Controller::registerMethods(). */
  struct PropCtlFixture
  {
    std::shared_ptr<MockAuthService> m_auth = std::make_shared<MockAuthService>();
    std::shared_ptr<MockHistoryService> m_history = std::make_shared<MockHistoryService>();
    std::shared_ptr<PropertyService> m_service = propctlMakeService();
    std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
    std::thread m_serverThread;
    int m_port = -1;
    std::unique_ptr<httplib::Client> m_client;

    PropCtlFixture()
    {
    }

    ~PropCtlFixture()
    {
      if (m_server != nullptr) { m_server->stop(); }
      if (m_serverThread.joinable()) { m_serverThread.join(); }
    }

    PropCtlFixture(const PropCtlFixture &) = delete;
    PropCtlFixture &operator=(const PropCtlFixture &) = delete;

    void seedProperty(const std::string &name, bool runtimeChange = true, std::int32_t value = 5)
    {
      m_service->getOrCreate<std::int32_t>(name, "instance", "klass", "process", "", runtimeChange, value);
    }

    void startServing(Controller &controller)
    {
      controller.registerMethods(m_server);
      m_port = m_server->bind_to_any_port("127.0.0.1");
      if (m_port <= 0) { return; }
      m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
      const auto deadline = std::chrono::steady_clock::now() + propctlStartupTimeout;
      while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (!m_server->is_running()) { return; }
      m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
      m_client->set_connection_timeout(std::chrono::seconds(10));
      m_client->set_read_timeout(std::chrono::seconds(10));
    }

    bool isReady() const { return m_client != nullptr && m_server->is_running(); }
  };

}// namespace

TEST_CASE("PropertyController allPropertiesOfGet returns matching properties", "[property_controller]")
{
  PropCtlFixture fixture;
  fixture.seedProperty("myProp");
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance", propctlAuthHeaders(propctlTokenUser));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.find("myProp") != std::string::npos);
}

TEST_CASE("PropertyController allPropertiesOfGet returns 401 without user", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
}

TEST_CASE("PropertyController allPropertiesOfGet returns 401 for user without groups", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance", propctlAuthHeaders(propctlTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
}

TEST_CASE("PropertyController allPropertiesOfGet returns 403 for non-json", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto headers = propctlAuthHeaders(propctlTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Get("/properties/process/klass/instance", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
}

TEST_CASE("PropertyController propertyOfGet returns the serialized property", "[property_controller]")
{
  PropCtlFixture fixture;
  fixture.seedProperty("myProp");
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance/myProp", propctlAuthHeaders(propctlTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body.find("myProp") != std::string::npos);
}

TEST_CASE("PropertyController propertyOfGet returns 405 for an unknown property", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance/missing", propctlAuthHeaders(propctlTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 405);
}

TEST_CASE("PropertyController propertyOfGet returns 401 without user", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Get("/properties/process/klass/instance/myProp");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
}

TEST_CASE("PropertyController propertyOfGet returns 403 for non-json", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto headers = propctlAuthHeaders(propctlTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Get("/properties/process/klass/instance/myProp", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
}

TEST_CASE("PropertyController updatePropertyPut updates the value and historizes", "[property_controller]")
{
  PropCtlFixture fixture;
  fixture.seedProperty("myProp");
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  REQUIRE_CALL(*fixture.m_history, historizeOf(ANY(std::vector<HistoryEntry>))).TIMES(1);

  auto response = fixture.m_client->Put("/properties/process/klass/instance/myProp",
    propctlAuthHeaders(propctlTokenAdmin),
    R"({"value":"42"})",
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(fixture.m_service->get("myProp", "instance", "klass", "process")->toString() == "42");
}

TEST_CASE("PropertyController updatePropertyPut returns 401 without user", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put(
    "/properties/process/klass/instance/myProp", {}, R"({"value":"42"})", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
}

TEST_CASE("PropertyController updatePropertyPut returns 401 for a non-admin user", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put("/properties/process/klass/instance/myProp",
    propctlAuthHeaders(propctlTokenUser),
    R"({"value":"42"})",
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
}

TEST_CASE("PropertyController updatePropertyPut returns 403 for non-json", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put("/properties/process/klass/instance/myProp",
    propctlAuthHeaders(propctlTokenAdmin),
    R"({"value":"42"})",
    "text/plain");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
}

TEST_CASE("PropertyController updatePropertyPut returns 405 for an unknown property", "[property_controller]")
{
  PropCtlFixture fixture;
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put("/properties/process/klass/instance/missing",
    propctlAuthHeaders(propctlTokenAdmin),
    R"({"value":"42"})",
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 405);
}

TEST_CASE("PropertyController updatePropertyPut returns 405 for a read-only property", "[property_controller]")
{
  PropCtlFixture fixture;
  fixture.seedProperty("readOnly", false, 5);
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put("/properties/process/klass/instance/readOnly",
    propctlAuthHeaders(propctlTokenAdmin),
    R"({"value":"42"})",
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 405);
}

TEST_CASE("PropertyController updatePropertyPut returns 405 for an invalid body", "[property_controller]")
{
  PropCtlFixture fixture;
  fixture.seedProperty("myProp");
  PropertyController controller(fixture.m_service, fixture.m_auth, fixture.m_history);
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());
  ALLOW_CALL(*fixture.m_auth, onAccessOf(ANY(const UserTokenLogin &))).LR_RETURN(propctlResolveToken(_1));

  auto response = fixture.m_client->Put(
    "/properties/process/klass/instance/myProp", propctlAuthHeaders(propctlTokenAdmin), "not-json", "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 405);
}

}// namespace lintel
