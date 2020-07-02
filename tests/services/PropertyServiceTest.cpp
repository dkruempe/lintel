#define CATCH_CONFIG_MAIN
#include "services/PropertyService.h"
#include "repositories/NoPropertyRepository.h"
#include <catch2/catch.hpp>

void createExampleProperties(PropertyService &propertyService) {
  std::shared_ptr<Property<int32_t>> intProperty =
      propertyService.getOrCreate<int32_t>("intProperty", "instanceName",
                                           "testClass", "processName", 4712);
  std::shared_ptr<Property<std::string>> stringProperty =
      propertyService.getOrCreate<std::string>("stringProperty", "instanceName",
                                               "testClass", "processName",
                                               "Hallo Welt!");
  std::shared_ptr<Property<double>> doubleProperty =
      propertyService.getOrCreate<double>("doubleProperty", "instanceName",
                                          "testClass", "processName", 3.421);
  std::shared_ptr<Property<std::string>> emptyStringProperty =
      propertyService.getOrCreate<std::string>(
          "emptyStringProperty", "instanceName", "testClass", "processName");
}

class PropertyExampleClass : public AbstractService<PropertyExampleClass> {
public:
  explicit PropertyExampleClass(PropertyService &propertyService)
      : AbstractService("testProcess", "testInstance") {
    LOAD_PROPERTIES();
  }
  DEFINE_PROPERTY(string, std::string, "Ich bin eine Test Property");
  DEFINE_PROPERTY(enable, bool, false);
};

TEST_CASE("test example service with properties") {
  NoPropertyRepository noPropertyRepository;
  PropertyService propertyService(noPropertyRepository, "testProcess");
  PropertyExampleClass propertyExampleClass(propertyService);
  REQUIRE(propertyService.allProperties().size() == 2);
  REQUIRE(propertyExampleClass.enable->getValue() == false);
  auto propertyBase = propertyService.get(
      "enable", "testInstance", "PropertyExampleClass", "testProcess");
  std::shared_ptr<Property<bool>> property =
      std::static_pointer_cast<Property<bool>>(propertyBase);
  propertyService.changeValueOf<bool>(property, true);
  REQUIRE(property->getValue() == true);
  REQUIRE(propertyExampleClass.enable->getValue() == true);

  propertyService.changeValueOf<std::string>(
      propertyExampleClass.string, "Ich liebe dich Anna Krümpelmann <3!");
  REQUIRE(propertyExampleClass.string->getValue() ==
          "Ich liebe dich Anna Krümpelmann <3!");
  auto stringProperty = propertyService.get(
      "string", "testInstance", "PropertyExampleClass", "testProcess");
  REQUIRE(stringProperty->toString() == "Ich liebe dich Anna Krümpelmann <3!");
}

TEST_CASE("test create/get of PropertyService") {
  NoPropertyRepository noPropertyRepository;
  PropertyService propertyService(noPropertyRepository, "main");
  createExampleProperties(propertyService);
  REQUIRE(propertyService.allProperties().size() == 4);
}
TEST_CASE("test setValue for Property") {
  NoPropertyRepository noPropertyRepository;
  PropertyService propertyService(noPropertyRepository, "main");
  createExampleProperties(propertyService);
  auto intProperty = propertyService.getOrCreate<int32_t>(
      "intProperty", "instanceName", "class", "processName", 4713);
  propertyService.changeValueOf<int32_t>(intProperty, 4711);
  REQUIRE(intProperty->getValue() == 4711);
  auto properties = propertyService.allProperties();
  for (const auto &property : properties) {
    if (property->getName() == "intProperty" &&
        property->getClassName() == "class") {
      REQUIRE(property->toString() == "4711");
    }
  }
}
TEST_CASE("test create with no defined default value") {
  NoPropertyRepository noPropertyRepository;
  PropertyService propertyService(noPropertyRepository, "main");
  createExampleProperties(propertyService);
  auto property = propertyService.getOrCreate<std::string>(
      "emptyStringProperty", "instanceName", "processName", "not empty");
  REQUIRE(property->getValue().empty());
}