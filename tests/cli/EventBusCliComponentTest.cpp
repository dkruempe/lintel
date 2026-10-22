#include <catch2/catch_all.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <httplib.h>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/components/EventBusCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace lintel {
namespace {

  // `ebCli` prefix: unity builds merge several test files into one anonymous
  // namespace, so a generic capture or backend name would collide.
  class ebCliCoutCapture
  {
  public:
    ebCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

    ~ebCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

    ebCliCoutCapture(const ebCliCoutCapture &) = delete;
    ebCliCoutCapture &operator=(const ebCliCoutCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  class ebCliCerrCapture
  {
  public:
    ebCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

    ~ebCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

    ebCliCerrCapture(const ebCliCerrCapture &) = delete;
    ebCliCerrCapture &operator=(const ebCliCerrCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  /**
   * `EventBusApi` has no virtual seam, so the component is driven against a
   * loopback HTTP server that records every request and answers with a canned
   * body.
   */
  class ebCliBackend
  {
  public:
    ebCliBackend()
    {
      // Record in the route handlers, not in a pre-routing handler: the
      // pre-routing hook runs before httplib has read the request body, so it
      // only ever sees an empty request.body.
      const auto record = [this](const httplib::Request &request, httplib::Response &response) {
        paths.push_back(request.path);
        methods.emplace_back(request.method);
        bodies.push_back(request.body);
        response.status = m_status;
        response.set_content(m_responseBody, "application/json");
      };
      m_server.Get(".*", record);
      m_server.Post(".*", record);
      m_server.Put(".*", record);
      m_server.Delete(".*", record);
      m_port = m_server.bind_to_any_port("127.0.0.1");
      m_thread = std::thread([this]() { m_server.listen_after_bind(); });
      // Server::stop() is a no-op until the accept loop is up.
      const auto ebCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (!m_server.is_running() && std::chrono::steady_clock::now() < ebCliDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    ~ebCliBackend()
    {
      m_server.stop();
      if (m_thread.joinable()) { m_thread.join(); }
    }

    ebCliBackend(const ebCliBackend &) = delete;
    ebCliBackend &operator=(const ebCliBackend &) = delete;

    void answerWith(const std::string &body) { m_responseBody = body; }

    void setStatus(int status) { m_status = status; }

    [[nodiscard]] int port() const { return m_port; }

    [[nodiscard]] bool running() const { return m_server.is_running(); }

    std::vector<std::string> paths;
    std::vector<std::string> methods;
    std::vector<std::string> bodies;

  private:
    httplib::Server m_server;
    std::thread m_thread;
    int m_port{ -1 };
    std::string m_responseBody{ "[]" };
    int m_status{ 200 };
  };

  /** Client provider whose only client entry points at the loopback backend. */
  std::shared_ptr<ClientProvider> ebCliProviderOf(int port)
  {
    static const std::string kEbCliAbsentConfigDir = "/nonexistent/lintel-eb-cli-test-config";
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(EnvironmentConfiguration::ConfigDirectory, kEbCliAbsentConfigDir);
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

  constexpr char ebCliOneBusBody[] = R"([{)"
                                     R"("NAME":"orders",)"
                                     R"("AVAILABLE":true,)"
                                     R"("BUS_CAPACITY":8,)"
                                     R"("SUBSCRIBER_CAPACITY":8,)"
                                     R"("MAX_TOPICS":8,)"
                                     R"("MAX_SUBSCRIBERS":8,)"
                                     R"("MAX_PRIORITIES":4,)"
                                     R"("PUB_POLICY":"Fail",)"
                                     R"("STICKY":false,)"
                                     R"("STICKY_TTL_MS":60000,)"
                                     R"("DROPPED":3,)"
                                     R"("TOPIC_COUNT":2,)"
                                     R"("SUBSCRIBER_COUNT":1,)"
                                     R"("BUS_DEPTH":0,)"
                                     R"("SEQUENCE":7)"
                                     R"(}])";

}// namespace

TEST_CASE("EventBusCliComponent: onShowMenu prints the no-submenu notice")
{
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(1));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onShowMenu();

  REQUIRE(ebCliOut.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: onMenu returns false for an unknown component")
{
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(1));
  EventBusCliComponent ebCliComponent(ebCliApi);

  REQUIRE(ebCliComponent.onMenu("History") == false);
}

TEST_CASE("EventBusCliComponent: onHelp prints the header and every command")
{
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(1));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onHelp();
  const std::string ebCliHelp = ebCliOut.str();

  REQUIRE(ebCliHelp.find("Help Menu of EventBus [Eb]:") != std::string::npos);
  REQUIRE(ebCliHelp.find("show_event_buses") != std::string::npos);
  REQUIRE(ebCliHelp.find("publish_event") != std::string::npos);
  REQUIRE(ebCliHelp.find("event_queue_depth") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: allCommandsOf returns the registered commands")
{
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(1));
  EventBusCliComponent ebCliComponent(ebCliApi);

  const std::vector<std::string> ebCliCommands = ebCliComponent.allCommandsOf();
  const auto ebCliHas = [&ebCliCommands](const std::string &command) {
    return std::find(ebCliCommands.begin(), ebCliCommands.end(), command) != ebCliCommands.end();
  };

  REQUIRE(ebCliCommands.size() == 7);
  REQUIRE(ebCliHas("show_event_buses"));
  REQUIRE(ebCliHas("show_event_bus"));
  REQUIRE(ebCliHas("publish_event"));
  REQUIRE(ebCliHas("subscribe_event"));
  REQUIRE(ebCliHas("unsubscribe_event"));
  REQUIRE(ebCliHas("event_queue_depth"));
  REQUIRE(ebCliHas("reset_event_bus"));
}

TEST_CASE("EventBusCliComponent: the component registers itself under name and alias")
{
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(1));
  EventBusCliComponent ebCliComponent(ebCliApi);

  REQUIRE(ebCliComponent.getName() == "EventBus");
  REQUIRE(ebCliComponent.getAlias() == "Eb");
  REQUIRE(ebCliComponent.onExit() == true);
}

TEST_CASE("EventBusCliComponent: show_event_buses prints the configuration and the counters")
{
  ebCliBackend ebCliServer;
  REQUIRE(ebCliServer.running());
  ebCliServer.answerWith(ebCliOneBusBody);
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "show_event_buses", {});
  const std::string ebCliPrinted = ebCliOut.str();

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/buses");
  REQUIRE(ebCliServer.methods[0] == "GET");
  REQUIRE(ebCliPrinted.find("BusCapacity") != std::string::npos);
  REQUIRE(ebCliPrinted.find("orders") != std::string::npos);
  REQUIRE(ebCliPrinted.find("Fail") != std::string::npos);
  REQUIRE(ebCliPrinted.find("yes") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: show_event_bus asks for one bus")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"NAME":"orders","AVAILABLE":true,"BUS_CAPACITY":8,"DROPPED":1})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "show_event_bus", { "--bus", "orders" });
  const std::string ebCliPrinted = ebCliOut.str();

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/bus/orders");
  REQUIRE(ebCliPrinted.find("orders") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: show_event_bus reports an unknown bus")
{
  ebCliBackend ebCliServer;
  ebCliServer.setStatus(404);
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "show_event_bus", { "-b", "unknown" });

  REQUIRE(ebCliOut.str().find("ERROR") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: publish_event posts the event and prints the result")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"BUS":"orders","PRIORITY":"High","PUBLISHED":true,"SEQUENCE":7,)"
                         R"("TARGET":"BUS","TOPIC":"orders.created","TYPE":"OrderCreated"})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{},
    "publish_event",
    { "--bus",
      "orders",
      "--topic",
      "orders.created",
      "--type",
      "OrderCreated",
      "--payload",
      "42",
      "--priority",
      "High" });
  const std::string ebCliPrinted = ebCliOut.str();

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/publish/orders");
  REQUIRE(ebCliServer.methods[0] == "POST");
  REQUIRE(ebCliServer.bodies[0].find("\"TOPIC\":\"orders.created\"") != std::string::npos);
  REQUIRE(ebCliServer.bodies[0].find("\"TYPE\":\"OrderCreated\"") != std::string::npos);
  REQUIRE(ebCliServer.bodies[0].find("\"PAYLOAD\":\"42\"") != std::string::npos);
  REQUIRE(ebCliServer.bodies[0].find("\"PRIORITY\":\"High\"") != std::string::npos);
  REQUIRE(ebCliServer.bodies[0].find("\"TARGET\":\"BUS\"") != std::string::npos);
  REQUIRE(ebCliPrinted.find("Published event orders.created/OrderCreated on event bus orders") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: publish_event --to-subscribers broadcasts")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"PUBLISHED":true})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(
    UserDto{}, "publish_event", { "-b", "orders", "-t", "orders.created", "-ty", "OrderCreated", "-ts" });

  REQUIRE(ebCliServer.bodies[0].find("\"TARGET\":\"SUBSCRIBERS\"") != std::string::npos);
  REQUIRE(ebCliOut.str().find("Published event") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: publish_event without a topic prints an error")
{
  ebCliBackend ebCliServer;
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCerrCapture ebCliErr;
  ebCliComponent.onCommand(UserDto{}, "publish_event", { "--bus", "orders", "--type", "OrderCreated" });

  REQUIRE(ebCliServer.paths.empty());
  REQUIRE(ebCliErr.str().find("ERROR") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: publish_event reports a dropped event")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"PUBLISHED":false})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "publish_event", { "-b", "orders", "-t", "t", "-ty", "T" });

  REQUIRE(ebCliOut.str().find("was dropped") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: subscribe_event posts the subscription")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"BUS":"orders","SUBSCRIBER":"worker","SUBSCRIBED":true,"TOPIC":"orders.created"})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(
    UserDto{}, "subscribe_event", { "--bus", "orders", "--topic", "orders.created", "--subscriber", "worker" });

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/subscribe/orders");
  REQUIRE(ebCliServer.methods[0] == "POST");
  REQUIRE(ebCliServer.bodies[0].find("\"SUBSCRIBER\":\"worker\"") != std::string::npos);
  REQUIRE(ebCliOut.str().find("Subscribed worker to orders.created on event bus orders") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: unsubscribe_event deletes the subscription")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"BUS":"orders","SUBSCRIBER":"worker","TOPIC":"orders.created","UNSUBSCRIBED":true})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "unsubscribe_event", { "-b", "orders", "-t", "orders.created", "-s", "worker" });

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/subscribe/orders");
  REQUIRE(ebCliServer.methods[0] == "DELETE");
  REQUIRE(ebCliOut.str().find("Unsubscribed worker from orders.created on event bus orders") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: event_queue_depth prints the depth")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"BUS":"orders","DEPTH":2,"SUBSCRIBER":"worker"})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "event_queue_depth", { "--bus", "orders", "--subscriber", "worker" });

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/depth/orders/worker");
  REQUIRE(ebCliOut.str().find("Depth of subscriber worker on event bus orders: 2") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: reset_event_bus posts the reset")
{
  ebCliBackend ebCliServer;
  ebCliServer.answerWith(R"({"BUS":"orders","RESET":true})");
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "reset_event_bus", { "-b", "orders" });

  REQUIRE(ebCliServer.paths.size() == 1);
  REQUIRE(ebCliServer.paths[0] == "/eventBus/reset/orders");
  REQUIRE(ebCliServer.methods[0] == "POST");
  REQUIRE(ebCliOut.str().find("Reset event bus orders") != std::string::npos);
}

TEST_CASE("EventBusCliComponent: an unknown command never reaches the api")
{
  ebCliBackend ebCliServer;
  auto ebCliApi = std::make_shared<EventBusApi>(ebCliProviderOf(ebCliServer.port()));
  EventBusCliComponent ebCliComponent(ebCliApi);

  ebCliCoutCapture ebCliOut;
  ebCliComponent.onCommand(UserDto{}, "show_buses", { "-b", "ignored" });

  REQUIRE(ebCliServer.paths.empty());
  REQUIRE(ebCliOut.str().empty());
}
}// namespace lintel
