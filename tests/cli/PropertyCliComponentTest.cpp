#include <catch2/catch_all.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/PropertyBase.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/components/PropertyCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/controller/PropertyDto.h"
#include "lintel/features/property/factories/PropertyFactory.h"

namespace lintel {
namespace {

  /** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
  class propCliCoutCapture
  {
  public:
    propCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

    ~propCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

    propCliCoutCapture(const propCliCoutCapture &) = delete;
    propCliCoutCapture &operator=(const propCliCoutCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  /** RAII redirector for `std::cerr`, used for the error paths of onCommand(). */
  class propCliCerrCapture
  {
  public:
    propCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

    ~propCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

    propCliCerrCapture(const propCliCerrCapture &) = delete;
    propCliCerrCapture &operator=(const propCliCerrCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  /**
   * `PropertyApi` has no virtual seam, so the component is driven against a loopback HTTP server
   * that records method, path and body of every request and answers with a canned body.
   */
  class propCliBackend
  {
  public:
    propCliBackend()
    {
      m_server.set_pre_routing_handler([this](const httplib::Request &request, httplib::Response &response) {
        paths.push_back(request.path);
        methods.emplace_back(request.method);
        bodies.emplace_back();
        if (request.method == "POST" || request.method == "PUT") { return httplib::Server::HandlerResponse::Unhandled; }
        response.status = m_responseStatus;
        response.set_content(m_responseBody, "application/json");
        return httplib::Server::HandlerResponse::Handled;
      });
      const auto propCliAnswer = [this](const httplib::Request &request, httplib::Response &response) {
        if (!bodies.empty()) { bodies.back() = request.body; }
        response.status = m_responseStatus;
        response.set_content(m_responseBody, "application/json");
      };
      m_server.Post(R"(.*)", propCliAnswer);
      m_server.Put(R"(.*)", propCliAnswer);
      m_port = m_server.bind_to_any_port("127.0.0.1");
      m_thread = std::thread([this]() { m_server.listen_after_bind(); });
      const auto propCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (!m_server.is_running() && std::chrono::steady_clock::now() < propCliDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    ~propCliBackend()
    {
      m_server.stop();
      if (m_thread.joinable()) { m_thread.join(); }
    }

    propCliBackend(const propCliBackend &) = delete;
    propCliBackend &operator=(const propCliBackend &) = delete;

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

    [[nodiscard]] bool running() const { return m_server.is_running(); }

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

  /** Client provider whose only client entry points at the loopback backend. */
  std::shared_ptr<ClientProvider> propCliProviderOf(int port)
  {
    static const std::string kPropCliAbsentConfigDir = "/nonexistent/lintel-prop-cli-test-config";
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(EnvironmentConfiguration::ConfigDirectory, kPropCliAbsentConfigDir);
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

  std::shared_ptr<PropertyBase> propCliSampleProperty()
  {
    return PropertyFactory::Create("myProp", "instance", "klass", "process", "int32_t", "5", "a description", true);
  }

  /** @return the JSON body of a single-property endpoint (GET /properties/.../name) */
  std::string propCliOnePropertyBody()
  {
    PropertyDto dto(propCliSampleProperty());
    return dto.JsonSerializable::serialize();
  }

  /** @return the JSON body of a property-list endpoint */
  std::string propCliOnePropertiesBody()
  {
    PropertiesDto dtos(std::vector<std::shared_ptr<PropertyBase>>{ propCliSampleProperty() });
    return dtos.JsonSerializable::serialize();
  }

}// namespace

TEST_CASE("PropertyCliComponent: the component registers itself under name and alias")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  REQUIRE(component.getName() == "Property");
  REQUIRE(component.getAlias() == "Prop");
}

TEST_CASE("PropertyCliComponent: onHelp prints the header and every command")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onHelp();
  const std::string help = out.str();

  REQUIRE(help.find("Help Menu of Property [Prop]:") != std::string::npos);
  REQUIRE(help.find("show_properties") != std::string::npos);
  REQUIRE(help.find("show_property") != std::string::npos);
  REQUIRE(help.find("update_property") != std::string::npos);
  // all three commands must use the same dash spelling for the option
  REQUIRE(help.find("--process-name, -p") != std::string::npos);
  REQUIRE(help.find("--class-name, -c") != std::string::npos);
  REQUIRE(help.find("--process_name") == std::string::npos);
  REQUIRE(help.find("--class_name") == std::string::npos);
}

TEST_CASE("PropertyCliComponent: printCommandList lists every registered command")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.printCommandList({ "exit" });
  const std::string list = out.str();

  REQUIRE(list.find("show_properties") != std::string::npos);
  REQUIRE(list.find("show_property") != std::string::npos);
  REQUIRE(list.find("update_property") != std::string::npos);
  REQUIRE(list.find("exit") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: allCommandsOf returns the registered commands")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  REQUIRE(component.allCommandsOf().size() == 3);
}

TEST_CASE("PropertyCliComponent: onShowMenu prints the no-submenu notice")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onShowMenu();

  REQUIRE(out.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: onMenu accepts any component and onExit returns true")
{
  auto api = std::make_shared<PropertyApi>(propCliProviderOf(1));
  PropertyCliComponent component(api);

  REQUIRE(component.onMenu("Property") == true);
  REQUIRE(component.onExit() == true);
}

TEST_CASE("PropertyCliComponent: show_properties without flags requests all properties")
{
  propCliBackend server;
  REQUIRE(server.port() > 0);
  REQUIRE(server.running());
  server.answerWith(propCliOnePropertiesBody());

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onCommand(UserDto{}, "show_properties", {});
  const std::string printed = out.str();

  REQUIRE(server.paths.size() == 1);
  REQUIRE(server.paths[0] == "/properties/.*/.*/.*");
  REQUIRE(server.methods[0] == "GET");
  REQUIRE(printed.find("Process") != std::string::npos);
  REQUIRE(printed.find("myProp") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: show_properties forwards the name filters")
{
  propCliBackend server;
  server.answerWith(propCliOnePropertiesBody());

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onCommand(
    UserDto{}, "show_properties", { "--process-name", "proc", "--class-name", "cls", "--instance-name", "inst" });

  REQUIRE(server.paths.size() == 1);
  REQUIRE(server.paths[0] == "/properties/proc/cls/inst");
}

TEST_CASE("PropertyCliComponent: show_property prints the requested property")
{
  propCliBackend server;
  server.answerWith(propCliOnePropertyBody());

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onCommand(UserDto{},
    "show_property",
    { "--process-name", "proc", "--class-name", "cls", "--instance-name", "inst", "--name", "myProp" });
  const std::string printed = out.str();

  REQUIRE(server.paths.size() == 1);
  REQUIRE(server.paths[0] == "/properties/proc/cls/inst/myProp");
  REQUIRE(printed.find("myProp") != std::string::npos);
  REQUIRE(printed.find("Data Storage") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: show_property reports an absent property on stderr")
{
  propCliBackend server;
  server.answerWithStatus(404, "");

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCerrCapture err;
  component.onCommand(UserDto{},
    "show_property",
    { "--process-name", "proc", "--class-name", "cls", "--instance-name", "inst", "--name", "missing" });

  REQUIRE(err.str().find("not preset") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: update_property updates and prints before/after")
{
  propCliBackend server;
  server.answerWith(propCliOnePropertyBody());

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCoutCapture out;
  component.onCommand(UserDto{},
    "update_property",
    { "--process-name",
      "proc",
      "--class-name",
      "cls",
      "--instance-name",
      "inst",
      "--value",
      "42",
      "--name",
      "myProp" });
  const std::string printed = out.str();

  REQUIRE(server.paths.size() == 3);
  REQUIRE(server.methods[0] == "GET");
  REQUIRE(server.methods[1] == "PUT");
  REQUIRE(server.methods[2] == "GET");
  REQUIRE(server.bodies[1].find("42") != std::string::npos);
  REQUIRE(printed.find("Before Update") != std::string::npos);
  REQUIRE(printed.find("After Update") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: update_property reports an absent property on stderr")
{
  propCliBackend server;
  server.answerWithStatus(404, "");

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCerrCapture err;
  component.onCommand(UserDto{},
    "update_property",
    { "--process-name",
      "proc",
      "--class-name",
      "cls",
      "--instance-name",
      "inst",
      "--value",
      "42",
      "--name",
      "missing" });

  REQUIRE(server.paths.size() == 1);
  REQUIRE(err.str().find("not preset") != std::string::npos);
}

TEST_CASE("PropertyCliComponent: an unknown command performs no request")
{
  propCliBackend server;

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  component.onCommand(UserDto{}, "does_not_exist", {});
  REQUIRE(server.paths.empty());
}

TEST_CASE("PropertyCliComponent: a failing api call is reported as an error")
{
  propCliBackend server;
  server.answerWithStatus(401, "");

  auto api = std::make_shared<PropertyApi>(propCliProviderOf(server.port()));
  PropertyCliComponent component(api);

  propCliCerrCapture err;
  component.onCommand(UserDto{}, "show_properties", {});

  REQUIRE(err.str().find("ERROR") != std::string::npos);
}

}// namespace lintel
