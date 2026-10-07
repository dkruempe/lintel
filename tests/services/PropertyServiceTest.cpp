#include <lintel/core/services/PropertyRegistration.h>
#include <lintel/features/property/services/PropertyService.h>

#include <catch2/catch_all.hpp>

void createExampleProperties(PropertyService &propertyService) {
    std::shared_ptr<Property<int32_t>> intProperty =
            propertyService.getOrCreate<int32_t>("intProperty", "instanceName",
                                                 "testClass", "processName", "", true,
                                                 4712);
    REQUIRE(intProperty->getValue() == 4712);
    std::shared_ptr<Property<std::string>> stringProperty =
            propertyService.getOrCreate<std::string>("stringProperty", "instanceName",
                                                     "testClass", "processName", "",
                                                     true, "Hallo Welt!");
    REQUIRE(stringProperty->getValue() == "Hallo Welt!");
    std::shared_ptr<Property<double>> doubleProperty =
            propertyService.getOrCreate<double>("doubleProperty", "instanceName",
                                                "testClass", "processName", "", true,
                                                3.421);
    REQUIRE(doubleProperty->getValue() == 3.421);
    std::shared_ptr<Property<std::string>> emptyStringProperty =
            propertyService.getOrCreate<std::string>("emptyStringProperty",
                                                     "instanceName", "testClass",
                                                     "processName", "", true);
    REQUIRE(emptyStringProperty->getValue() == "");
}

class PropertyExampleClass : public PropertyRegistration<PropertyExampleClass> {
public:
    explicit PropertyExampleClass(
            const std::shared_ptr<PropertyService> &propertyService)
            : PropertyRegistration("testProcess", "testInstance") {
        LOAD_PROPERTIES();
    }

    DEFINE_PROPERTY(string, std::string, "I am a test property", "", true);
    DEFINE_PROPERTY(enable, bool, false, "", true);
};

TEST_CASE("test example service with properties") {
    std::vector<std::shared_ptr<PropertyRepository>> repositories = {};
    std::vector<std::shared_ptr<AbstractServiceInterface>>
            abstractServiceInterfaces = {};
    std::shared_ptr<PropertyService> propertyService =
            std::make_shared<PropertyService>(repositories,
                                              abstractServiceInterfaces);
    PropertyExampleClass propertyExampleClass(propertyService);
    REQUIRE(propertyService->allOf().size() == 2);
    REQUIRE(!propertyExampleClass.enable->getValue());
    auto propertyBase = propertyService->get(
            "enable", "testInstance", "PropertyExampleClass", "testProcess");
    std::shared_ptr<Property<bool>> property =
            std::static_pointer_cast<Property<bool>>(propertyBase);
    propertyService->changeValueOf<bool>(property, true);
    REQUIRE(property->getValue() == true);
    REQUIRE(propertyExampleClass.enable->getValue() == true);

    propertyService->changeValueOf<std::string>(
            propertyExampleClass.string, "Ich bin ein Beispieltext <3!");
    REQUIRE(propertyExampleClass.string->getValue() ==
            "Ich bin ein Beispieltext <3!");
    auto stringProperty = propertyService->get(
            "string", "testInstance", "PropertyExampleClass", "testProcess");
    REQUIRE(stringProperty->toString() == "Ich bin ein Beispieltext <3!");
}

TEST_CASE("test create/get of PropertyService") {
    PropertyService propertyService({}, {});
    createExampleProperties(propertyService);
    REQUIRE(propertyService.allOf().size() == 4);
}

TEST_CASE("test setValue for Property") {
    PropertyService propertyService({}, {});
    createExampleProperties(propertyService);
    auto intProperty = propertyService.getOrCreate<int32_t>(
            "intProperty", "instanceName", "class", "processName", "", true, 4713);
    propertyService.changeValueOf<int32_t>(intProperty, 4711);
    REQUIRE(intProperty->getValue() == 4711);
    auto properties = propertyService.allOf();
    for (const auto &property: properties) {
        if (property->getName() == "intProperty" &&
            property->getClassName() == "class") {
            REQUIRE(property->toString() == "4711");
        }
    }
}

TEST_CASE("test create with no defined default value") {
    PropertyService propertyService({}, {});
    createExampleProperties(propertyService);
    auto property = propertyService.getOrCreate<std::string>(
            "emptyStringProperty", "instanceName", "className", "processName", "",
            true, "not empty");
    REQUIRE(!property->getValue().empty());
}