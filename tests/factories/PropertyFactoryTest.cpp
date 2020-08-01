#define CATCH_CONFIG_MAIN
#include <base_library/factories/PropertyFactory.h>
#include <base_library/models/Property.h>
#include <catch2/catch.hpp>

TEST_CASE("basic propertyFactoryTest") {
  auto property =
      PropertyFactory::Create("testProperty", "testInstance", "testClass",
                              "testProcess", "int32_t", "4711");
  REQUIRE(property != nullptr);
  std::shared_ptr<Property<int32_t>> intProperty =
      std::static_pointer_cast<Property<int32_t>>(property);
  REQUIRE(intProperty->getValue() == 4711);
  REQUIRE(intProperty->toString() == "4711");
  REQUIRE(intProperty->getType() == "int32_t");
  REQUIRE(intProperty->getInstanceName() == "testInstance");
  REQUIRE(intProperty->getClassName() == "testClass");
  REQUIRE(intProperty->getProcessName() == "testProcess");
  REQUIRE(intProperty->getName() == "testProperty");

  auto createInt8Property =
      PropertyFactory::Create("int8Property", "testInstance", "testClass",
                              "testProcess", "int8_t", "3");
  std::shared_ptr<Property<int8_t>> int8Property =
      std::static_pointer_cast<Property<int8_t>>(createInt8Property);
  REQUIRE(int8Property->getValue() == 3);
  REQUIRE(int8Property->toString() == "3");
  REQUIRE(int8Property->getType() == "int8_t");
  REQUIRE(int8Property->getInstanceName() == "testInstance");
  REQUIRE(int8Property->getClassName() == "testClass");
  REQUIRE(int8Property->getProcessName() == "testProcess");
  REQUIRE(int8Property->getName() == "int8Property");

  auto createStringProperty =
      PropertyFactory::Create("stringProperty", "testInstance", "testClass",
                              "testProcess", "std::string", "Hallo Welt!");
  std::shared_ptr<Property<std::string>> stringProperty =
      std::static_pointer_cast<Property<std::string>>(createStringProperty);
  REQUIRE(stringProperty->getValue() == "Hallo Welt!");
  REQUIRE(stringProperty->toString() == "Hallo Welt!");
  REQUIRE(stringProperty->getType() == "std::string");
  REQUIRE(stringProperty->getInstanceName() == "testInstance");
  REQUIRE(stringProperty->getClassName() == "testClass");
  REQUIRE(stringProperty->getProcessName() == "testProcess");
  REQUIRE(stringProperty->getName() == "stringProperty");
}