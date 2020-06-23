#define CATCH_CONFIG_MAIN
#include "models/Property.h"
#include <catch2/catch.hpp>

TEST_CASE("basic test of property") {
  Property<int32_t> intProperty("testProperty", "testInstance", "testProcess",
                                4711);
  REQUIRE(intProperty.getValue() == 4711);
  REQUIRE(intProperty.toString() == "4711");
  REQUIRE(intProperty.getType() == "int32_t");
  REQUIRE(intProperty.getInstanceName() == "testInstance");
  REQUIRE(intProperty.getProcessName() == "testProcess");
  REQUIRE(intProperty.getName() == "testProperty");

  Property<int8_t> int8Property("int8Property", "testInstance", "testProcess",
                                3);
  REQUIRE(int8Property.getValue() == 3);
  REQUIRE(int8Property.toString() == "3");
  REQUIRE(int8Property.getType() == "int8_t");
  REQUIRE(int8Property.getInstanceName() == "testInstance");
  REQUIRE(int8Property.getProcessName() == "testProcess");
  REQUIRE(int8Property.getName() == "int8Property");

  Property<std::string> stringProperty("stringProperty", "testInstance",
                                       "testProcess", "Hallo Welt!");
  REQUIRE(stringProperty.getValue() == "Hallo Welt!");
  REQUIRE(stringProperty.toString() == "Hallo Welt!");
  REQUIRE(stringProperty.getType() == "std::string");
  REQUIRE(stringProperty.getInstanceName() == "testInstance");
  REQUIRE(stringProperty.getProcessName() == "testProcess");
  REQUIRE(stringProperty.getName() == "stringProperty");
}