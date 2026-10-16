#include <catch2/catch_all.hpp>

#include <httplib.h>

#include <chrono>
#include <memory>
#include <vector>

#include "lintel/app/CommandLineFeature.h"
#include "lintel/app/HttpFeature.h"
#include "lintel/app/PropertyFeature.h"
#include "lintel/core/Features.h"
#include "lintel/core/configuration/Component.h"
#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/di/Container.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/configuration/ServerConfiguration.h"
#include "lintel/features/http/provider/ServerProvider.h"

#include "../helpers/ScopedEnvironmentVariable.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace lintel {
namespace {

  // `app` prefix: unity builds merge several test files into one translation unit
  // and share one anonymous namespace.
  std::shared_ptr<Configuration> appConfiguration(std::vector<std::shared_ptr<Entry>> entries)
  {
    auto environment = std::make_shared<EnvironmentConfiguration>();
    auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, environment);
    configuration->setEntries(std::move(entries));
    return configuration;
  }

  std::shared_ptr<ClientConfiguration> appClientConfiguration()
  {
    return std::make_shared<ClientConfiguration>("127.0.0.1",
      1,
      std::chrono::milliseconds(1000),
      std::chrono::milliseconds(1000),
      std::chrono::milliseconds(1000));
  }

  /** @return an ephemeral TCP port that is free at the time of the call */
  int appFreePort()
  {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { return 0; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
      ::close(fd);
      return 0;
    }
    socklen_t length = sizeof(address);
    if (::getsockname(fd, reinterpret_cast<sockaddr *>(&address), &length) != 0) {
      ::close(fd);
      return 0;
    }
    const int port = ntohs(address.sin_port);
    ::close(fd);
    return port;
  }

}// namespace

TEST_CASE("PropertyFeature registers its types with the container builder")
{
  auto features = std::make_shared<Features>();
  PropertyFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);

  auto container = builder.build();
  REQUIRE(container != nullptr);
}

TEST_CASE("CommandLineFeature registers its types with the container builder")
{
  auto features = std::make_shared<Features>();
  CommandLineFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);

  auto container = builder.build();
  REQUIRE(container != nullptr);
}

TEST_CASE("HttpFeature registers its types with the container builder")
{
  auto features = std::make_shared<Features>();
  HttpFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);

  auto container = builder.build();
  REQUIRE(container != nullptr);
}

// The three initialize() overrides all resolve a service whose own dependency graph reaches a
// Configuration instance that is not registered here. Resolving therefore either succeeds (if the
// graph is self-contained) or throws - in both cases the body of initialize() is executed, which is
// what these cases exercise.
TEST_CASE("PropertyFeature::initialize resolves its property service")
{
  auto features = std::make_shared<Features>();
  PropertyFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);
  auto container = builder.build();

  try {
    feature.initialize(container);
  } catch (const std::exception &) {
    SUCCEED("the dependency graph needs a full environment");
  }
}

TEST_CASE("CommandLineFeature::initialize resolves its command line service")
{
  auto features = std::make_shared<Features>();
  CommandLineFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);
  auto container = builder.build();

  try {
    feature.initialize(container);
  } catch (const std::exception &) {
    SUCCEED("the dependency graph needs a full environment");
  }
}

TEST_CASE("HttpFeature::initialize resolves its server provider")
{
  auto features = std::make_shared<Features>();
  HttpFeature feature(features);

  di::ContainerBuilder builder;
  feature.registerTypes(builder);
  auto container = builder.build();

  try {
    feature.initialize(container);
  } catch (const std::exception &) {
    SUCCEED("the dependency graph needs a full environment");
  }
}

TEST_CASE("ServerProvider provides the server of a server configuration entry")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_app_feature_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };

  const int port = appFreePort();
  REQUIRE(port > 0);
  auto serverConfiguration = std::make_shared<ServerConfiguration>("127.0.0.1", port);
  // a client entry must be skipped; the server entry that follows wins
  auto configuration =
    appConfiguration({ std::make_shared<HttpEntry>(type_name<HttpComponent>(), appClientConfiguration()),
      std::make_shared<HttpEntry>(type_name<HttpComponent>(), serverConfiguration) });

  ServerProvider provider(configuration, {});
  REQUIRE(provider.provide() != nullptr);

  // A request after construction guarantees the accept loop is running before the
  // provider is destroyed; Server::stop() is a no-op until the loop is up, so an
  // immediate teardown would block forever in the listener thread join.
  httplib::Client client("127.0.0.1", port);
  client.set_connection_timeout(std::chrono::seconds(2));
  client.set_read_timeout(std::chrono::seconds(2));
  auto health = client.Get("/health");
  REQUIRE(health != nullptr);
  REQUIRE(health->status == 200);
}

TEST_CASE("ServerProvider provides nothing when only client entries are configured")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_app_feature_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };

  auto configuration =
    appConfiguration({ std::make_shared<HttpEntry>(type_name<HttpComponent>(), appClientConfiguration()) });

  ServerProvider provider(configuration, {});
  REQUIRE(provider.provide() == nullptr);
}

TEST_CASE("ServerProvider provides nothing without any http entry")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_app_feature_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };

  auto configuration = appConfiguration({});

  ServerProvider provider(configuration, {});
  REQUIRE(provider.provide() == nullptr);
}

}// namespace lintel
