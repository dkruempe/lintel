#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/configuration/ConfigurationException.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/EventBusComponent.h>
#include <lintel/features/base/configuration/EventBusEntry.h>
#include <lintel/features/base/configuration/LoggerComponent.h>
#include <lintel/features/base/configuration/LoggerConfiguration.h>
#include <lintel/features/base/configuration/LoggerEntry.h>
#include <lintel/features/base/configuration/LoggerPathConfiguration.h>
#include <lintel/features/base/configuration/LoggerSinkConfiguration.h>
#include <lintel/features/base/configuration/MessageQueueComponent.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/configuration/ProcessComponent.h>
#include <lintel/features/base/configuration/ProcessEntry.h>
#include <lintel/features/base/events/EventBus.h>

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>

TEST_CASE("LoggerComponent: parse Logger with ConsoleSink")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
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

TEST_CASE("LoggerComponent: parse Logger with RotatingFileSink")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
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

TEST_CASE("LoggerComponent: parse multiple sinks")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
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

TEST_CASE("LoggerComponent: parse with custom pattern")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
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

TEST_CASE("LoggerComponent: parse Path")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  ScopedEnvironmentVariable home{ "HOME", "/tmp" };
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

TEST_CASE("LoggerComponent: parse empty child returns empty")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers></Loggers>)";
  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.empty());
}

TEST_CASE("LoggerComponent: throw on missing process_name")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers>
        <Logger level="info" async="false">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing level")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers>
        <Logger process_name="main" async="false">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing async")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info">
            <LoggerSink type="ConsoleSink"/>
        </Logger>
    </Loggers>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on missing sink type")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info" async="false">
            <LoggerSink/>
        </Logger>
    </Loggers>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("LoggerComponent: throw on unknown sink type")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  LoggerComponent component(envConfig);

  std::string xml = R"(<Loggers>
        <Logger process_name="main" level="info" async="false">
            <LoggerSink type="UnknownSinkType"/>
        </Logger>
    </Loggers>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("ProcessComponent: parse single process")
{
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

TEST_CASE("ProcessComponent: parse process with autoRestart")
{
  ProcessComponent component;

  std::string xml = R"(<Processes>
        <Process name="/usr/bin/worker" args="" autoRestart="true" maxRestarts="5"/>
    </Processes>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto process = std::static_pointer_cast<ProcessEntry>(entries[0])->getProcess();
  REQUIRE(process->getPath() == "/usr/bin/worker");
}

TEST_CASE("ProcessComponent: parse process with args")
{
  ProcessComponent component;

  std::string xml = R"(<Processes>
        <Process name="/usr/bin/myapp" args="--verbose,--debug" autoRestart="true" maxRestarts="3"/>
    </Processes>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);
}

TEST_CASE("ProcessComponent: parse process group")
{
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

TEST_CASE("ProcessComponent: parse multiple entries")
{
  ProcessComponent component;

  std::string xml = R"(<Processes>
        <Process name="/usr/bin/app1" args="" autoRestart="false" maxRestarts="0"/>
        <Process name="/usr/bin/app2" args="" autoRestart="true" maxRestarts="3"/>
    </Processes>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);
}

TEST_CASE("ProcessComponent: empty Processes returns empty")
{
  ProcessComponent component;
  std::string xml = R"(<Processes></Processes>)";
  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.empty());
}

TEST_CASE("ProcessComponent: throw on missing name")
{
  ProcessComponent component;
  std::string xml = R"(<Processes>
        <Process args="" autoRestart="false" maxRestarts="0"/>
    </Processes>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("ProcessComponent: throw on missing autoRestart")
{
  ProcessComponent component;
  std::string xml = R"(<Processes>
        <Process name="/test" args="" maxRestarts="0"/>
    </Processes>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("ProcessComponent: parse automation policy attributes")
{
  ProcessComponent component;

  std::string xml = R"(<Processes>
        <Process name="/usr/bin/app" args="" autoRestart="true" maxRestarts="10"
            restartDelay="100" restartDelayMax="5000" minUptime="2000" maxRestartRate="5"
            restartWindow="60000" restartInterval="300000" activeFrom="22" activeTo="6"
            cpuNotify="80" memNotify="1048576"/>
    </Processes>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto process = std::static_pointer_cast<ProcessEntry>(entries[0])->getProcess();
  REQUIRE(process->getRestartDelay() == std::chrono::milliseconds(100));
  REQUIRE(process->getRestartDelayMax() == std::chrono::milliseconds(5000));
  REQUIRE(process->getMinUptime() == std::chrono::milliseconds(2000));
  REQUIRE(process->getMaxRestartRate() == 5);
  REQUIRE(process->getRestartWindow() == std::chrono::milliseconds(60000));
  REQUIRE(process->getRestartInterval() == std::chrono::milliseconds(300000));
  REQUIRE(process->getActiveFromHour().value() == 22);
  REQUIRE(process->getActiveToHour().value() == 6);
  REQUIRE(process->hasActiveWindow());
  REQUIRE(process->getCpuNotify().has_value());
  REQUIRE(process->getCpuNotify().value() == 80.0);
  REQUIRE(process->getMemoryNotify().has_value());
  REQUIRE(process->getMemoryNotify().value() == 1048576ULL);
}

TEST_CASE("ProcessComponent: automation policy defaults when attributes absent")
{
  ProcessComponent component;

  std::string xml = R"(<Processes>
        <Process name="/usr/bin/app" args="" autoRestart="true" maxRestarts="5"/>
    </Processes>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto process = std::static_pointer_cast<ProcessEntry>(entries[0])->getProcess();
  REQUIRE(process->getRestartDelay() == std::chrono::milliseconds(0));
  REQUIRE(process->getMaxRestartRate() == -1);
  REQUIRE_FALSE(process->hasActiveWindow());
  REQUIRE_FALSE(process->getCpuNotify().has_value());
  REQUIRE_FALSE(process->getMemoryNotify().has_value());
}

TEST_CASE("MessageQueueComponent: parse single queue")
{
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

TEST_CASE("MessageQueueComponent: parse multiple queues")
{
  MessageQueueComponent component;

  std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" name="queue1" max_messages="50"/>
        <MessageQueue process_name="main" name="queue2" max_messages="200"/>
    </MessageQueues>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);
}

TEST_CASE("MessageQueueComponent: empty MessageQueues returns empty")
{
  MessageQueueComponent component;
  std::string xml = R"(<MessageQueues></MessageQueues>)";
  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.empty());
}

TEST_CASE("MessageQueueComponent: throw on missing name")
{
  MessageQueueComponent component;
  std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" max_messages="100"/>
    </MessageQueues>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("MessageQueueComponent: throw on missing process_name")
{
  MessageQueueComponent component;
  std::string xml = R"(<MessageQueues>
        <MessageQueue name="test" max_messages="100"/>
    </MessageQueues>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("MessageQueueComponent: throw on missing max_messages")
{
  MessageQueueComponent component;
  std::string xml = R"(<MessageQueues>
        <MessageQueue process_name="main" name="test"/>
    </MessageQueues>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("EventBusComponent: parse single bus")
{
  EventBusComponent component;

  std::string xml = R"(<EventBuses>
        <EventBus name="main" segment="shm_eventbus" bus_capacity="64"
                  subscriber_capacity="32" max_topics="4" max_subscribers="2"/>
    </EventBuses>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto entry = std::static_pointer_cast<EventBusEntry>(entries[0]);
  REQUIRE(entry->getName() == "main");
  REQUIRE(entry->getSegment() == "shm_eventbus");
  REQUIRE(entry->getBusCapacity() == 64);
  REQUIRE(entry->getSubscriberCapacity() == 32);
  REQUIRE(entry->getMaxTopics() == 4);
  REQUIRE(entry->getMaxSubscribers() == 2);
}

TEST_CASE("EventBusComponent: defaults for omitted capacities")
{
  EventBusComponent component;

  std::string xml = R"(<EventBuses>
        <EventBus name="main" segment="shm_eventbus"/>
    </EventBuses>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);

  auto entry = std::static_pointer_cast<EventBusEntry>(entries[0]);
  REQUIRE(entry->getName() == "main");
  REQUIRE(entry->getSegment() == "shm_eventbus");
  REQUIRE(entry->getBusCapacity() == 128);
  REQUIRE(entry->getSubscriberCapacity() == 128);
  REQUIRE(entry->getMaxTopics() == EventBusLimits::MAX_TOPICS);
  REQUIRE(entry->getMaxSubscribers() == EventBusLimits::MAX_SUBSCRIBERS);
}

TEST_CASE("EventBusComponent: parse multiple buses")
{
  EventBusComponent component;

  std::string xml = R"(<EventBuses>
        <EventBus name="main" segment="shm_a" bus_capacity="16"/>
        <EventBus name="worker" segment="shm_b" bus_capacity="32"/>
    </EventBuses>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);

  auto first = std::static_pointer_cast<EventBusEntry>(entries[0]);
  auto second = std::static_pointer_cast<EventBusEntry>(entries[1]);
  REQUIRE(first->getName() == "main");
  REQUIRE(first->getSegment() == "shm_a");
  REQUIRE(first->getBusCapacity() == 16);
  REQUIRE(second->getName() == "worker");
  REQUIRE(second->getSegment() == "shm_b");
  REQUIRE(second->getBusCapacity() == 32);
}

TEST_CASE("EventBusComponent: empty EventBuses returns empty")
{
  EventBusComponent component;
  std::string xml = R"(<EventBuses></EventBuses>)";
  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.empty());
}

TEST_CASE("EventBusComponent: throw on missing name")
{
  EventBusComponent component;
  std::string xml = R"(<EventBuses>
        <EventBus segment="shm_eventbus"/>
    </EventBuses>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("EventBusComponent: throw on missing segment")
{
  EventBusComponent component;
  std::string xml = R"(<EventBuses>
        <EventBus name="main"/>
    </EventBuses>)";
  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}
