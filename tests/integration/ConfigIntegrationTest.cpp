#include <lintel/features/base/configuration/Component.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/ConfigurationComponentBuilder.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/http/configuration/HttpComponent.h>
#include <lintel/features/http/configuration/HttpEntry.h>
#include <lintel/core/utils/TypeName.h>

#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "../helpers/ScopedEnvironmentVariable.h"

TEST_CASE("Component: getConfigRoot returns configured root name") {
    class TestComponent : public Component {
    public:
        TestComponent() : Component("TestRoot") {}
        std::vector<std::shared_ptr<Entry>> parse(
            const std::string &, const std::string &, int32_t) override {
            return {};
        }
    };
    TestComponent comp;
    REQUIRE(comp.getConfigRoot() == "TestRoot");
}

TEST_CASE("Component: convertToBytes parses size strings") {
    class TestComponent : public Component {
    public:
        TestComponent() : Component("Test") {}
        std::vector<std::shared_ptr<Entry>> parse(
            const std::string &, const std::string &, int32_t) override {
            return {};
        }
        std::size_t testConvert(const std::string &s) { return convertToBytes(s); }
    };
    TestComponent comp;
    REQUIRE(comp.testConvert("1024") == 1024);
    REQUIRE(comp.testConvert("1kB") == 1000);
    REQUIRE(comp.testConvert("1MB") == 1000000);
    REQUIRE(comp.testConvert("1GB") == 1000000000);
    REQUIRE(comp.testConvert("2MB") == 2000000);
}

TEST_CASE("HttpComponent: parse server configuration") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    auto &serverConfig = httpEntry->getServerConfiguration();
    REQUIRE(serverConfig->getHost() == "0.0.0.0");
    REQUIRE(serverConfig->getPort() == 8080);
}

TEST_CASE("HttpComponent: parse client configuration") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Client host="localhost" port="9090" connection_timeout="5000"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    auto &clientConfig = httpEntry->getClientConfiguration();
    REQUIRE(clientConfig->getHost() == "localhost");
    REQUIRE(clientConfig->getPort() == 9090);
}

TEST_CASE("HttpComponent: parse both server and client") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="8080"/>
        <Client host="localhost" port="9090"/>
    </HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 2);
}

TEST_CASE("HttpComponent: parse with timeouts") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="8080" read_timeout="3000" write_timeout="5000" idle_timeout="60000"/>
    </HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    auto &config = httpEntry->getServerConfiguration();
    REQUIRE(config->getReadTimeOut().count() == 3000);
    REQUIRE(config->getWriteTimeOut().count() == 5000);
    REQUIRE(config->getIdleInterval().count() == 60000);
}

TEST_CASE("HttpComponent: require_tls defaults to false") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE_FALSE(httpEntry->getServerConfiguration()->isTlsRequired());
}

TEST_CASE("HttpComponent: require_tls true is parsed") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080" require_tls="true"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->isTlsRequired());
}

TEST_CASE("HttpComponent: require_tls case-insensitive value") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080" require_tls="TRUE"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->isTlsRequired());
}

TEST_CASE("HttpComponent: require_tls 1 is parsed as true") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080" require_tls="1"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->isTlsRequired());
}

TEST_CASE("HttpComponent: trusted_proxies defaults to empty") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->getTrustedProxies().empty());
}

TEST_CASE("HttpComponent: trusted_proxies parsed with trimming") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="8080" trusted_proxies="10.0.0.1, 192.168.0.0/16,2001:db8::/32"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    auto &proxies = httpEntry->getServerConfiguration()->getTrustedProxies();
    REQUIRE(proxies.size() == 3);
    REQUIRE(proxies[0] == "10.0.0.1");
    REQUIRE(proxies[1] == "192.168.0.0/16");
    REQUIRE(proxies[2] == "2001:db8::/32");
}

TEST_CASE("HttpComponent: relative cert/key paths resolved against config dir") {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory,
                         "/tmp/example_cfg");
    HttpComponent httpComp(envConfig);
    std::string xml = R"(<HttpHost>
        <Server host="0.0.0.0" port="8080" cert_path="certs/server.crt" key_path="certs/server.key"/>
        <Client host="localhost" port="8080" cert_path="certs/client.crt" key_path="certs/client.key"/>
    </HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 2);

    auto serverEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(serverEntry->getServerConfiguration()->getCertFile() ==
            "/tmp/example_cfg/certs/server.crt");
    REQUIRE(serverEntry->getServerConfiguration()->getKeyFile() ==
            "/tmp/example_cfg/certs/server.key");

    auto clientEntry = std::static_pointer_cast<HttpEntry>(entries[1]);
    REQUIRE(clientEntry->getClientConfiguration()->getCertFile() ==
            "/tmp/example_cfg/certs/client.crt");
    REQUIRE(clientEntry->getClientConfiguration()->getKeyFile() ==
            "/tmp/example_cfg/certs/client.key");
}

TEST_CASE("HttpComponent: absolute cert/key paths kept unchanged") {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory,
                         "/tmp/example_cfg");
    HttpComponent httpComp(envConfig);
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="443" cert_path="/etc/ssl/server.pem" key_path="/etc/ssl/private/server.key"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->getCertFile() ==
            "/etc/ssl/server.pem");
    REQUIRE(httpEntry->getServerConfiguration()->getKeyFile() ==
            "/etc/ssl/private/server.key");
}

TEST_CASE("HttpComponent: without env config relative cert/key paths kept") {
    HttpComponent httpComp;
    std::string xml = R"(<HttpHost><Server host="0.0.0.0" port="443" cert_path="certs/server.crt" key_path="certs/server.key"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    REQUIRE(httpEntry->getServerConfiguration()->getCertFile() ==
            "certs/server.crt");
    REQUIRE(httpEntry->getServerConfiguration()->getKeyFile() ==
            "certs/server.key");
}

TEST_CASE("HttpComponent: client ca_cert_path resolved against config dir") {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::Environment::ConfigDirectory,
                         "/tmp/example_cfg");
    HttpComponent httpComp(envConfig);
    std::string xml = R"(<HttpHost><Client host="localhost" port="8080" cert_path="certs/client.crt" key_path="certs/client.key" ca_cert_path="certs/server.crt"/></HttpHost>)";

    auto entries = httpComp.parse(xml, "test.xml", 0);
    auto httpEntry = std::static_pointer_cast<HttpEntry>(entries[0]);
    auto &clientConfig = httpEntry->getClientConfiguration();
    REQUIRE(clientConfig->getCertFile() == "/tmp/example_cfg/certs/client.crt");
    REQUIRE(clientConfig->getKeyFile() == "/tmp/example_cfg/certs/client.key");
    REQUIRE(clientConfig->getCaCertFile() ==
            "/tmp/example_cfg/certs/server.crt");
}

TEST_CASE("Configuration: setEntries and query by type") {
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };

    class TestEntry : public Entry {
    public:
        TestEntry(std::string_view component, int value)
            : Entry(component), m_value(value) {}
        int getValue() const { return m_value; }
    private:
        int m_value;
    };

    Configuration config({}, std::make_shared<EnvironmentConfiguration>());
    config.setEntries({
        std::make_shared<TestEntry>("TestComponent", 42),
        std::make_shared<TestEntry>("TestComponent", 99),
        std::make_shared<TestEntry>("OtherComponent", 7),
    });

    class TestComponent : public Component {
    public:
        TestComponent() : Component("Test") {}
        std::vector<std::shared_ptr<Entry>> parse(
            const std::string &, const std::string &, int32_t) override {
            return {};
        }
    };

    auto entries = config.configurationOf<TestComponent>();
    REQUIRE(entries.size() == 2);
}

TEST_CASE("Configuration: with type_name matching") {
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };

    class TestComponent : public Component {
    public:
        TestComponent() : Component("Test") {}
        std::vector<std::shared_ptr<Entry>> parse(
            const std::string &, const std::string &, int32_t) override {
            return {};
        }
    };

    class TestEntry : public Entry {
    public:
        TestEntry() : Entry(type_name<TestComponent>()) {}
    };

    Configuration config({}, std::make_shared<EnvironmentConfiguration>());
    config.setEntries({std::make_shared<TestEntry>()});

    auto entries = config.configurationOf<TestComponent>();
    REQUIRE(entries.size() == 1);
}

TEST_CASE("Entry: getConfigurationParserComponent") {
    class CustomEntry : public Entry {
    public:
        explicit CustomEntry(std::string_view comp) : Entry(comp) {}
    };
    CustomEntry entry("MyComponent");
    REQUIRE(entry.getConfigurationParserComponent() == "MyComponent");
}
