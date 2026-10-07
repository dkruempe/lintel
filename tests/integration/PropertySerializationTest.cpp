#include <lintel/features/property/models/Property.h>
#include <lintel/features/property/strategies/XMLConfigSerializationStrategy.h>
#include <lintel/features/property/factories/PropertyFactory.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>

TEST_CASE("XMLConfigSerializationStrategy: serialize properties to XML") {
    XMLConfigSerializationStrategy strategy;

    auto prop1 = std::make_shared<Property<int32_t>>(
        "threads", "__DEFAULT__", "SchedulerService", "main", 4, "Number of threads", false);
    auto prop2 = std::make_shared<Property<std::string>>(
        "name", "__DEFAULT__", "AppService", "main", "MyApp", "Application name", true);
    auto prop3 = std::make_shared<Property<bool>>(
        "enabled", "__DEFAULT__", "FeatureService", "main", true, "Feature enabled", true);

    std::vector<std::shared_ptr<PropertyBase>> properties = {prop1, prop2, prop3};
    std::string xml = strategy.serialize(properties);

    REQUIRE(xml.find("threads") != std::string::npos);
    REQUIRE(xml.find("SchedulerService") != std::string::npos);
    REQUIRE(xml.find("4") != std::string::npos);
    REQUIRE(xml.find("name") != std::string::npos);
    REQUIRE(xml.find("MyApp") != std::string::npos);
    REQUIRE(xml.find("enabled") != std::string::npos);
    REQUIRE(xml.find("true") != std::string::npos);
}

TEST_CASE("XMLConfigSerializationStrategy: deserialize from XML string") {
    std::string xml = R"(<?xml version="1.0" encoding="utf-8"?>
<Properties>
    <Property name="maxConnections" type="int32_t" value="100"
              process="main" class="DatabaseService" instance="__DEFAULT__"/>
    <Property name="hostname" type="std::string" value="localhost"
              process="main" class="DatabaseService" instance="__DEFAULT__"/>
    <Property name="debugMode" type="bool" value="true"
              process="main" class="LoggerService" instance="__DEFAULT__"/>
</Properties>)";

    XMLConfigSerializationStrategy strategy;
    auto properties = strategy.deserialize("test.xml", xml);

    REQUIRE(properties.size() == 3);

    auto &maxConnectionsProperty = properties[0];
    REQUIRE(maxConnectionsProperty->getName() == "maxConnections");
    REQUIRE(maxConnectionsProperty->getClassName() == "DatabaseService");
    REQUIRE(maxConnectionsProperty->getProcessName() == "main");
    REQUIRE(maxConnectionsProperty->getType() == "int32_t");
    REQUIRE(maxConnectionsProperty->toString() == "100");

    auto &hostnameProperty = properties[1];
    REQUIRE(hostnameProperty->getName() == "hostname");
    REQUIRE(hostnameProperty->getType() == "std::string");
    REQUIRE(hostnameProperty->toString() == "localhost");

    auto &debugModeProperty = properties[2];
    REQUIRE(debugModeProperty->getName() == "debugMode");
    REQUIRE(debugModeProperty->getType() == "bool");
    REQUIRE(debugModeProperty->toString() == "true");
}

TEST_CASE("XMLConfigSerializationStrategy: roundtrip serialize and deserialize") {
    XMLConfigSerializationStrategy strategy;

    auto original = std::make_shared<Property<double>>(
        "pi", "inst1", "MathService", "main", 3.14159, "PI constant", true);

    std::vector<std::shared_ptr<PropertyBase>> props = {original};
    std::string xml = strategy.serialize(props);

    auto deserialized = strategy.deserialize("roundtrip.xml", xml);
    REQUIRE(deserialized.size() == 1);

    auto &loaded = deserialized[0];
    REQUIRE(loaded->getName() == "pi");
    REQUIRE(loaded->getClassName() == "MathService");
    REQUIRE(loaded->getProcessName() == "main");
    REQUIRE(loaded->getInstanceName() == "inst1");
    REQUIRE(loaded->getType() == "double");
    REQUIRE(loaded->toString() == "3.141590");
    // runtimeChange is not preserved through serialization
}

TEST_CASE("XMLConfigSerializationStrategy: roundtrip int32_t") {
    XMLConfigSerializationStrategy strategy;

    auto original = std::make_shared<Property<int32_t>>(
        "count", "inst", "TestClass", "test", 42, "description", true);

    std::string xml = strategy.serialize({original});
    auto deserialized = strategy.deserialize("test.xml", xml);

    REQUIRE(deserialized.size() == 1);
    auto typed = std::static_pointer_cast<Property<int32_t>>(deserialized[0]);
    REQUIRE(typed->getValue() == 42);
}

TEST_CASE("XMLConfigSerializationStrategy: deserialize from file") {
    XMLConfigSerializationStrategy strategy;

    auto tmpPath = std::filesystem::temp_directory_path() / "test_props_XXXXXX.xml";
    std::string tmpStr = tmpPath.string();
    {
        std::string xml = R"(<?xml version="1.0" encoding="utf-8"?>
<Properties>
    <Property name="testProp" type="int32_t" value="777"
              process="main" class="TestClass" instance="__DEFAULT__"/>
</Properties>)";
        std::ofstream ofs(tmpStr);
        ofs << xml;
    }

    auto properties = strategy.deserialize(tmpPath);
    REQUIRE(properties.size() == 1);
    REQUIRE(properties[0]->getName() == "testProp");
    REQUIRE(properties[0]->toString() == "777");

    std::filesystem::remove(tmpPath);
}

TEST_CASE("Property<int32_t>: basic operations") {
    Property<int32_t> prop("test", "instance", "Class", "process", 42, "desc", true);
    REQUIRE(prop.getValue() == 42);
    REQUIRE(prop.toString() == "42");
    REQUIRE(prop.getType() == "int32_t");
    REQUIRE(prop.isRuntimeChange() == true);

    PropertyBase &baseRef = prop;
    baseRef.setValueString("100");
    REQUIRE(prop.getValue() == 100);
}

TEST_CASE("Property<bool>: basic operations") {
    Property<bool> prop("flag", "instance", "Class", "process", false, "desc", true);
    REQUIRE(prop.getValue() == false);
    REQUIRE(prop.toString() == "false");

    PropertyBase &baseRef = prop;
    baseRef.setValueString("true");
    REQUIRE(prop.getValue() == true);
}

TEST_CASE("Property<std::string>: basic operations") {
    Property<std::string> prop("name", "instance", "Class", "process",
                               "initial", "desc", true);
    REQUIRE(prop.getValue() == "initial");
    REQUIRE(prop.toString() == "initial");

    PropertyBase &baseRef = prop;
    baseRef.setValueString("updated");
    REQUIRE(prop.getValue() == "updated");
}

TEST_CASE("Property<double>: basic operations") {
    Property<double> prop("val", "instance", "Class", "process", 3.14, "desc", true);
    REQUIRE(prop.getValue() == 3.14);

    PropertyBase &baseRef = prop;
    baseRef.setValueString("2.71");
    REQUIRE(prop.getValue() == 2.71);
}

TEST_CASE("Property<int64_t>: basic operations") {
    int64_t big = 9223372036854775807LL;
    Property<int64_t> prop("big", "instance", "Class", "process", big, "desc", true);
    REQUIRE(prop.getValue() == big);
    REQUIRE(prop.toString() == "9223372036854775807");
}

TEST_CASE("Property<uint32_t>: basic operations") {
    Property<uint32_t> prop("uval", "instance", "Class", "process",
                            4294967295U, "desc", true);
    REQUIRE(prop.getValue() == 4294967295U);
}

TEST_CASE("Property<bool>: runtime change flag") {
    Property<bool> prop("flag", "instance", "Class", "process", true, "desc", false);
    REQUIRE(prop.isRuntimeChange() == false);

    Property<bool> propRuntime("flag2", "instance", "Class", "process", true, "desc", true);
    REQUIRE(propRuntime.isRuntimeChange() == true);
}

TEST_CASE("Property: DataStorage is set correctly") {
    Property<int32_t> prop("test", "instance", "Class", "process", 1, "desc", true);
    DataStorage storage(PropertyRepositoryType::FILE_REPOSITORY, "test.cpp:42");
    prop.setDataStorage(storage);
    REQUIRE(prop.getDataStorage().getType() == PropertyRepositoryType::FILE_REPOSITORY);
    REQUIRE(prop.getDataStorage().getExtraInformation() == "test.cpp:42");
}
