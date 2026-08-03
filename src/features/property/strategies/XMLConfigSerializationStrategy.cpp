#include "base_library/features/property/strategies/XMLConfigSerializationStrategy.h"

#include <tinyxml2.h>

#include <vector>

#include "base_library/core/services/FileService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/property/factories/PropertyFactory.h"

#define CONFIG_ROOT "Properties"
#define ELEMENT_NAME "name"
#define PROCESS_ROOT "process"
#define CLASS_ROOT "class"
#define INSTANCE_ROOT "instance"
#define PROPERTY_ROOT "Property"
#define PROPERTY_TYPE "type"
#define PROPERTY_VALUE "value"

std::string XMLConfigSerializationStrategy::serialize(
        std::vector<std::shared_ptr<PropertyBase>> properties) {
    std::sort(properties.begin(), properties.end(),
              [](const std::shared_ptr<PropertyBase> &rhs, const std::shared_ptr<PropertyBase> &lhs) {
                  return *rhs < *lhs;
              });
    tinyxml2::XMLDocument document;
    tinyxml2::XMLPrinter printer;

    tinyxml2::XMLElement *element = document.NewElement(CONFIG_ROOT);
    for (const auto &property: properties) {
        tinyxml2::XMLElement *propertyElement = document.NewElement(PROPERTY_ROOT);
        propertyElement->SetAttribute(ELEMENT_NAME, property->getName().c_str());
        propertyElement->SetAttribute(PROPERTY_TYPE, property->getType().c_str());
        propertyElement->SetAttribute(PROPERTY_VALUE, property->toString().c_str());
        propertyElement->SetAttribute(PROCESS_ROOT,
                                      property->getProcessName().c_str());
        propertyElement->SetAttribute(CLASS_ROOT, property->getClassName().c_str());
        propertyElement->SetAttribute(INSTANCE_ROOT,
                                      property->getInstanceName().c_str());
        element->InsertEndChild(propertyElement);
    }
    document.InsertEndChild(element);

    document.Print(&printer);
    return std::string(printer.CStr());
}

std::vector<std::shared_ptr<PropertyBase>>
XMLConfigSerializationStrategy::deserialize(const std::filesystem::path &path) {
    FileService file(path);
    if (!file.exists()) {
        return {};
    }
    return deserialize(file.getName(), file.readFile());
}

std::vector<std::shared_ptr<PropertyBase>>
XMLConfigSerializationStrategy::deserialize(const std::string &fileName,
                                            const std::string &content) {
    std::vector<std::shared_ptr<PropertyBase>> properties;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode = document.FirstChildElement();
    if (rootNode == nullptr) {
        return properties;
    }

    for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement();
         propertyElement != nullptr;
         propertyElement = propertyElement->NextSiblingElement()) {
        if (std::string(propertyElement->Name()) != PROPERTY_ROOT) {
            continue;
        }
        // Attribute() returns nullptr for missing attributes; never hand that
        // to std::string (UB)
        auto attribute = [propertyElement](const char *name) -> std::string {
            const char *value = propertyElement->Attribute(name);
            return value == nullptr ? std::string() : std::string(value);
        };
        std::string processName = attribute(PROCESS_ROOT);
        std::string className = attribute(CLASS_ROOT);
        std::string instanceName = attribute(INSTANCE_ROOT);
        std::string propertyName = attribute(ELEMENT_NAME);
        std::string propertyType = attribute(PROPERTY_TYPE);
        std::string propertyValue = attribute(PROPERTY_VALUE);
        // default no description and runtime change not allowed => default set is
        // overwritten later via orignal properties
        auto property = PropertyFactory::Create(
                propertyName, instanceName, className, processName, propertyType,
                propertyValue, "", false);
        if (property == nullptr) {
            LOG_ERROR("XML property '{}' has unknown type '{}' and is skipped",
                      propertyName, propertyType);
            continue;
        }
        property->setDataStorage(DataStorage(
                PropertyRepositoryType::FILE_REPOSITORY,
                fileName + ":" + std::to_string(propertyElement->GetLineNum())));
        properties.push_back(property);
    }
    return properties;
}
