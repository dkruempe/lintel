#include <catch2/catch_all.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/PropertyBase.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/controllers/PropertyApi.h"
#include "lintel/features/http/provider/ClientProvider.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/controller/PropertyDto.h"
#include "lintel/features/property/factories/PropertyFactory.h"

namespace lintel {
namespace {

  /** Loopback HTTP backend recording requests and answering with a canned response. */
  class propApiBackend
  {
  public:
    propApiBackend()
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

    ~propApiBackend()
    {
      m_server.stop();
      if (m_thread.joinable()) { m_thread.join(); }
    }

    propApiBackend(const propApiBackend &) = delete;
    propApiBackend &operator=(const propApiBackend &) = delete;

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

  std::shared_ptr<ClientProvider> propApiProviderOf(int port)
  {
    static const std::string kPropApiAbsentConfigDir = "/nonexistent/lintel-prop-api-test-config";
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(EnvironmentConfiguration::ConfigDirectory, kPropApiAbsentConfigDir);
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

  std::shared_ptr<PropertyBase> propApiProperty(const std::string &name)
  {
    return PropertyFactory::Create(name, "instance", "klass", "process", "int32_t", "42", "", true);
  }

  std::string propApiListBody()
  {
    PropertiesDto dto(std::vector<std::shared_ptr<PropertyBase>>{ propApiProperty("alpha"), propApiProperty("beta") });
    return dto.JsonSerializable::serialize();
  }

  std::string propApiOneBody()
  {
    PropertyDto dto(propApiProperty("alpha"));
    return dto.JsonSerializable::serialize();
  }

}// namespace

TEST_CASE("PropertyApi: allOf returns the parsed properties")
{
  propApiBackend backend;
  backend.answerWith(propApiListBody());
  PropertyApi api(propApiProviderOf(backend.port()));

  const auto properties = api.allOf("process", "klass", "instance");
  REQUIRE(properties.size() == 2);
  REQUIRE(properties[0].getName() == "alpha");
  REQUIRE(properties[1].getName() == "beta");
  REQUIRE(backend.paths.back() == "/properties/process/klass/instance");
}

TEST_CASE("PropertyApi: allOf handles unauthorized, error and bad body")
{
  propApiBackend backend;
  PropertyApi api(propApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "[]");
  REQUIRE_THROWS_AS(api.allOf("process", "klass", "instance"), HttpUnauthorizedException);

  backend.answerWithStatus(500, "[]");
  REQUIRE(api.allOf("process", "klass", "instance").empty());

  backend.answerWithStatus(200, "not json");
  REQUIRE(api.allOf("process", "klass", "instance").empty());
}

TEST_CASE("PropertyApi: allOf returns empty when no server answers")
{
  PropertyApi api(propApiProviderOf(1));
  REQUIRE(api.allOf("process", "klass", "instance").empty());
}

TEST_CASE("PropertyApi: of returns the parsed property")
{
  propApiBackend backend;
  backend.answerWith(propApiOneBody());
  PropertyApi api(propApiProviderOf(backend.port()));

  const auto property = api.of("process", "klass", "instance", "alpha");
  REQUIRE(property.has_value());
  REQUIRE(property->getName() == "alpha");
  REQUIRE(backend.paths.back() == "/properties/process/klass/instance/alpha");
}

TEST_CASE("PropertyApi: of handles unauthorized, error and bad body")
{
  propApiBackend backend;
  PropertyApi api(propApiProviderOf(backend.port()));

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.of("process", "klass", "instance", "alpha"), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  REQUIRE_FALSE(api.of("process", "klass", "instance", "alpha").has_value());

  backend.answerWithStatus(200, "not json");
  REQUIRE_FALSE(api.of("process", "klass", "instance", "alpha").has_value());
}

TEST_CASE("PropertyApi: updateOf covers OK, unauthorized and error")
{
  propApiBackend backend;
  PropertyApi api(propApiProviderOf(backend.port()));
  PropertyDto dto(propApiProperty("alpha"));

  backend.answerWithStatus(200, "{}");
  REQUIRE(api.updateOf(dto, "43"));
  REQUIRE(backend.paths.back() == "/properties/process/klass/instance/alpha");
  REQUIRE(backend.methods.back() == "PUT");

  backend.answerWithStatus(401, "{}");
  REQUIRE_THROWS_AS(api.updateOf(dto, "43"), HttpUnauthorizedException);

  backend.answerWithStatus(500, "{}");
  REQUIRE_FALSE(api.updateOf(dto, "43"));
}

}// namespace lintel
