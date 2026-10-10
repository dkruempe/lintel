#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/http/configuration/HttpComponent.h>
#include <lintel/features/http/configuration/HttpEntry.h>

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>

// HttpComponent resolves relative certificate paths against the configuration directory. An unset
// path must survive that step unchanged: without the empty-path guard `std::filesystem::path("")`
// still gets joined, and every "no TLS" deployment would be pointed at the directory itself
// ("/tmp/example_cfg/") instead of at "no certificate at all".

TEST_CASE("HttpComponent: absent cert/key/ca paths stay empty with a config directory set")
{
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory, "/tmp/example_cfg");
  HttpComponent httpComp(envConfig);
  std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="443"/>
        <Client host="localhost" port="443"/>
    </HttpHost>)";

  auto entries = httpComp.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);

  auto serverEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
  REQUIRE(serverEntry->getServerConfiguration()->getCertFile().empty());
  REQUIRE(serverEntry->getServerConfiguration()->getKeyFile().empty());

  auto clientEntry = std::static_pointer_cast<HttpEntry>(entries[1]);
  REQUIRE(clientEntry->getClientConfiguration()->getCertFile().empty());
  REQUIRE(clientEntry->getClientConfiguration()->getKeyFile().empty());
  REQUIRE(clientEntry->getClientConfiguration()->getCaCertFile().empty());
}

TEST_CASE("HttpComponent: empty cert/key/ca attributes stay empty with a config directory set")
{
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory, "/tmp/example_cfg");
  HttpComponent httpComp(envConfig);
  std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="443" cert_path="" key_path=""/>
        <Client host="localhost" port="443" cert_path="" key_path="" ca_cert_path=""/>
    </HttpHost>)";

  auto entries = httpComp.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);

  auto serverEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
  REQUIRE(serverEntry->getServerConfiguration()->getCertFile().empty());
  REQUIRE(serverEntry->getServerConfiguration()->getKeyFile().empty());

  auto clientEntry = std::static_pointer_cast<HttpEntry>(entries[1]);
  REQUIRE(clientEntry->getClientConfiguration()->getCertFile().empty());
  REQUIRE(clientEntry->getClientConfiguration()->getKeyFile().empty());
  REQUIRE(clientEntry->getClientConfiguration()->getCaCertFile().empty());
}

// Counterpart of the two cases above: an explicitly configured relative path is still resolved, so
// the empty cases are not passing because the resolution itself is broken.
TEST_CASE("HttpComponent: a set cert path is still resolved against the config directory")
{
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory, "/tmp/example_cfg");
  HttpComponent httpComp(envConfig);
  std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="443" cert_path="certs/server.crt" key_path="certs/server.key"/>
    </HttpHost>)";

  auto entries = httpComp.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto serverEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
  REQUIRE(serverEntry->getServerConfiguration()->getCertFile() == "/tmp/example_cfg/certs/server.crt");
  REQUIRE(serverEntry->getServerConfiguration()->getKeyFile() == "/tmp/example_cfg/certs/server.key");
}