#include <base_library/core/configuration/LoggerComponent.h>
#include <base_library/core/configuration/ProcessComponent.h>
#include <base_library/core/configuration/MessageQueueComponent.h>
#include <base_library/core/configuration/EnvironmentConfiguration.h>
#include <base_library/core/configuration/LoggerEnrty.h>
#include <base_library/core/configuration/ProcessEntry.h>
#include <base_library/core/configuration/MessageQueueEntry.h>
#include <base_library/core/configuration/LoggerConfiguration.h>
#include <base_library/core/configuration/LoggerPathConfiguration.h>
#include <base_library/core/configuration/LoggerSinkConfiguration.h>
#include <base_library/core/configuration/ConfigurationException.h>
#include <base_library/core/utils/TypeName.h>

#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <filesystem>
#include <sstream>

TEST_CASE("LoggerComponent: parse Logger with ConsoleSink") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info" async="false">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto loggerEntry = std::static_pointer_cast<LoggerEntry>(entries[0]);
    REQUIRE(loggerEntry->isLoggerConfiguration());
    REQUIRE_FALSE(loggerEntry->isLoggerPathConfiguration());

    auto config = loggerEntry->getLoggerConfiguration();
    REQUIRE(config->getProcessName() == "main");
    REQUIRE(config->getLevel() == "info");

    auto sinks = config->getLoggerSinks();
    REQUIRE(sinks.size() == 1);
    REQUIRE(sinks[0].getType() == LoggerSinkConfiguration::ConsoleSink);
}

TEST_CASE("LoggerComponent: parse Logger with RotatingFileSink") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="worker" level="debug" async="true">
            <LoggerSink type="RotatingFileSink" size="5MB" max_files="10"/>
        </Logger>
    </Loggers>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto config = std::static_pointer_cast<LoggerEntry>(entries[0])->getLoggerConfiguration();
    REQUIRE(config->getProcessName() == "worker");
    REQUIRE(config->getLevel() == "debug");

    auto sinks = config->getLoggerSinks();
    REQUIRE(sinks.size() == 1);
    REQUIRE(sinks[0].getType() == LoggerSinkConfiguration::RotatingFileSink);
    REQUIRE(sinks[0].getMaxFiles() == 10);
}

TEST_CASE("LoggerComponent: parse multiple sinks") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="trace" async="false">
            <LoggerSink type="ConsoleSink"/>
            <LoggerSink type="TcpSink" connection="localhost" port="9999"/>
        </Logger>
    </Loggers>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto config = std::static_pointer_cast<LoggerEntry>(entries[0])->getLoggerConfiguration();
    auto sinks = config->getLoggerSinks();
    REQUIRE(sinks.size() == 2);
    REQUIRE(sinks[0].getType() == LoggerSinkConfiguration::ConsoleSink);
    REQUIRE(sinks[1].getType() == LoggerSinkConfiguration::TcpSink);
    REQUIRE(sinks[1].getConnection() == "localhost");
    REQUIRE(sinks[1].getPort() == 9999);
}

TEST_CASE("LoggerComponent: parse with custom pattern") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="warn" async="false" pattern="[%l] %v">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto config = std::static_pointer_cast<LoggerEntry>(entries[0])->getLoggerConfiguration();
    REQUIRE(config->getPattern() == "[%l] %v");
}

TEST_CASE("LoggerComponent: parse Path") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    setenv("HOME", "/tmp", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();

    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Path path="/tmp/test_logs" create_sub_dirs="true"/>
    </Loggers>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto loggerEntry = std::static_pointer_cast<LoggerEntry>(entries[0]);
    REQUIRE(loggerEntry->isLoggerPathConfiguration());

    auto pathConfig = loggerEntry->getLoggerPathConfiguration();
    REQUIRE(pathConfig->isCreateSubDirectories());
}

TEST_CASE("LoggerComponent: parse empty child returns empty") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers></Loggers>)";
    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.empty());
}

TEST_CASE("LoggerComponent: throw on missing process_name") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger level="info" async="false">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing level") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" async="false">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing async") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing sink type") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info" async="false">
            <LoggerSink/>
        </Logger>
    </Loggers>)";

    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on unknown sink type") {
    setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    LoggerComponent component(envConfig);

    std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info" async="false">
            <LoggerSink type="UnknownSinkType"/>
        </Logger>
    </Loggers>)";

    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("ProcessComponent: parse single process") {
    ProcessComponent component;

    std::string xml = R"(<Processes>
        <Process name="/usr/bin/myapp" args="" autoRestart="false" maxRestarts="0"/>
    </Processes>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto processEntry = std::static_pointer_cast<ProcessEntry>(entries[0]);
    auto process = processEntry->getProcess();
    REQUIRE(process->getPath() == "/usr/bin/myapp");
}

TEST_CASE("ProcessComponent: parse process with autoRestart") {
    ProcessComponent component;

    std::string xml = R"(<Processes>
        <Process name="/usr/bin/worker" args="" autoRestart="true" maxRestarts="5"/>
    </Processes>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto process = std::static_pointer_cast<ProcessEntry>(entries[0])->getProcess();
    REQUIRE(process->getPath() == "/usr/bin/worker");
}

TEST_CASE("ProcessComponent: parse process with args") {
    ProcessComponent component;

    std::string xml = R"(<Processes>
        <Process name="/usr/bin/myapp" args="--verbose,--debug" autoRestart="true" maxRestarts="3"/>
    </Processes>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);
}

TEST_CASE("ProcessComponent: parse process group") {
    ProcessComponent component;

    std::string xml = R"(<Processes>
        <ProcessGroup name="test_group">
            <Process name="/usr/bin/app1" args="" autoRestart="false" maxRestarts="0"/>
            <Process name="/usr/bin/app2" args="" autoRestart="false" maxRestarts="0"/>
        </ProcessGroup>
    </Processes>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto processEntry = std::static_pointer_cast<ProcessEntry>(entries[0]);
    auto group = processEntry->getProcessGroup();
    REQUIRE(group->getName() == "test_group");
}

TEST_CASE("ProcessComponent: parse multiple entries") {
    ProcessComponent component;

    std::string xml = R"(<Processes>
        <Process name="/usr/bin/app1" args="" autoRestart="false" maxRestarts="0"/>
        <Process name="/usr/bin/app2" args="" autoRestart="true" maxRestarts="3"/>
    </Processes>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 2);
}

TEST_CASE("ProcessComponent: empty Processes returns empty") {
    ProcessComponent component;
    std::string xml = R"(<Processes></Processes>)";
    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.empty());
}

TEST_CASE("ProcessComponent: throw on missing name") {
    ProcessComponent component;
    std::string xml = R"(<Processes>
        <Process args="" autoRestart="false" maxRestarts="0"/>
    </Processes>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("ProcessComponent: throw on missing autoRestart") {
    ProcessComponent component;
    std::string xml = R"(<Processes>
        <Process name="/test" args="" maxRestarts="0"/>
    </Processes>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("MessageQueueComponent: parse single queue") {
    MessageQueueComponent component;

    std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" name="commands" max_messages="100"/>
    </MessageQueues>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 1);

    auto entry = std::static_pointer_cast<MessageQueueEntry>(entries[0]);
    REQUIRE(entry->get_process_name() == "main");
    REQUIRE(entry->get_message_queue_name() == "commands");
    REQUIRE(entry->get_max_messages() == 100);
}

TEST_CASE("MessageQueueComponent: parse multiple queues") {
    MessageQueueComponent component;

    std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" name="queue1" max_messages="50"/>
        <MessageQueue process_name="main" name="queue2" max_messages="200"/>
    </MessageQueues>)";

    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.size() == 2);
}

TEST_CASE("MessageQueueComponent: empty MessageQueues returns empty") {
    MessageQueueComponent component;
    std::string xml = R"(<MessageQueues></MessageQueues>)";
    auto entries = component.parse(xml, "test.xml", 0);
    REQUIRE(entries.empty());
}

TEST_CASE("MessageQueueComponent: throw on missing name") {
    MessageQueueComponent component;
    std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" max_messages="100"/>
    </MessageQueues>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("MessageQueueComponent: throw on missing process_name") {
    MessageQueueComponent component;
    std::string xml = R"(<MessageQueues>
        <MessageQueue name="test" max_messages="100"/>
    </MessageQueues>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("MessageQueueComponent: throw on missing max_messages") {
    MessageQueueComponent component;
    std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" name="test"/>
    </MessageQueues>)";
    REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}
