#define CATCH_CONFIG_MAIN
#include "services/PropertyService.h"
#include <catch2/catch.hpp>

void createExampleProperties(PropertyService &propertyService) {
  std::shared_ptr<Property<int32_t>> intProperty =
      propertyService.getOrCreate<int32_t>("intProperty", "instanceName",
                                           "processName", 4712);
  std::shared_ptr<Property<std::string>> stringProperty =
      propertyService.getOrCreate<std::string>("stringProperty", "instanceName",
                                               "processName", "Hallo Welt!");
  std::shared_ptr<Property<double>> doubleProperty =
      propertyService.getOrCreate<double>("doubleProperty", "instanceName",
                                          "processName", 3.421);
  std::shared_ptr<Property<std::string>> emptyStringProperty =
      propertyService.getOrCreate<std::string>("emptyStringProperty",
                                               "instanceName", "processName");
}

TEST_CASE("test create/get of PropertyService") {
  PropertyService propertyService;
  createExampleProperties(propertyService);
  REQUIRE(propertyService.allProperties().size() == 4);
}
TEST_CASE("test setValue for Property") {
  PropertyService propertyService;
  createExampleProperties(propertyService);
  auto intProperty = propertyService.getOrCreate<int32_t>(
      "intProperty", "instanceName", "processName", 4713);
  propertyService.changeValueOf<int32_t>(intProperty, 4711);
  REQUIRE(intProperty->getValue() == 4711);
  auto properties = propertyService.allProperties();
  for (const auto &property : properties) {
    if (property->getName() == "intProperty") {
      REQUIRE(property->toString() == "4711");
    }
  }
}
TEST_CASE("test create with no defined default value") {
  PropertyService propertyService;
  createExampleProperties(propertyService);
  auto property = propertyService.getOrCreate<std::string>(
      "emptyStringProperty", "instanceName", "processName", "not empty");
  REQUIRE(property->getValue().empty());
}