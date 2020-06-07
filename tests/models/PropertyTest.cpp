#define CATCH_CONFIG_MAIN
#include "models/MutableProperty.h"
#include <catch2/catch.hpp>

TEST_CASE("basic test of property") {
  MutableProperty<int32_t> mutableIntProperty("testProperty", "testInstance",
                                              "testProcess", 4711);
  Property<int32_t> &intProperty = mutableIntProperty;
  REQUIRE(intProperty.getValue() == 4711);
  REQUIRE(intProperty.toString() == "4711");
  REQUIRE(intProperty.getType() == "int");
  REQUIRE(intProperty.getInstanceName() == "testInstance");
  REQUIRE(intProperty.getProcessName() == "testProcess");
  REQUIRE(intProperty.getName() == "testProperty");

  MutableProperty<int8_t> mutableInt8Property("int8Property", "testInstance",
                                              "testProcess", 3);
  Property<int8_t> &int8Property = mutableInt8Property;
  REQUIRE(int8Property.getValue() == 3);
  REQUIRE(int8Property.toString() == "3");
  REQUIRE(int8Property.getType() == "signed char");
  REQUIRE(int8Property.getInstanceName() == "testInstance");
  REQUIRE(int8Property.getProcessName() == "testProcess");
  REQUIRE(int8Property.getName() == "int8Property");

  MutableProperty<std::string> mutableStringProperty(
      "stringProperty", "testInstance", "testProcess", "Hallo Welt!");
  Property<std::string> &stringProperty = mutableStringProperty;
  REQUIRE(stringProperty.getValue() == "Hallo Welt!");
  REQUIRE(stringProperty.toString() == "Hallo Welt!");
  REQUIRE(stringProperty.getType() == "std::__1::basic_string<char>");
  REQUIRE(stringProperty.getInstanceName() == "testInstance");
  REQUIRE(stringProperty.getProcessName() == "testProcess");
  REQUIRE(stringProperty.getName() == "stringProperty");
}