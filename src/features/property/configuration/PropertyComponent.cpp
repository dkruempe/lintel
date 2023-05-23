#include "base_library/features/property/configuration/PropertyComponent.h"

#include <base_library/features/base/configuration/ConfigurationException.h>

#include "base_library/core/utils/TypeName.h"
#include "base_library/features/property/configuration/PropertyEntry.h"
#include "base_library/features/property/factories/PropertyFactory.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

PropertyComponent::Shapes PropertyComponent::shape{};

PropertyComponent::PropertyComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>> PropertyComponent::parse(
        const std::string &content, const std::string &fileName,
        const int32_t lineOffset) {
    std::vector<std::shared_ptr<Entry>> properties;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return properties;
    }

    for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement();
         propertyElement != nullptr;
         propertyElement = propertyElement->NextSiblingElement()) {
        if (std::strcmp(propertyElement->Name(), shape.PROPERTY_ROOT.c_str()) !=
            0) {
            continue;
        }
        const char *processName =
                propertyElement->Attribute(shape.PROCESS_ROOT.c_str());
        const char *className =
                propertyElement->Attribute(shape.CLASS_ROOT.c_str());
        const char *instanceName =
                propertyElement->Attribute(shape.INSTANCE_ROOT.c_str());
        const char *propertyName =
                propertyElement->Attribute(shape.ELEMENT_NAME.c_str());
        const char *propertyType =
                propertyElement->Attribute(shape.PROPERTY_TYPE.c_str());
        const char *propertyValue =
                propertyElement->Attribute(shape.PROPERTY_VALUE.c_str());

        int32_t lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

        if (processName == nullptr) {
            throw ConfigurationException(getConfigRoot(), "process name is null",
                                         lineNumber);
        }

        if (className == nullptr) {
            throw ConfigurationException(getConfigRoot(), "class name is null",
                                         lineNumber);
        }

        if (instanceName == nullptr) {
            throw ConfigurationException(getConfigRoot(), "instance name is null",
                                         lineNumber);
        }

        if (propertyName == nullptr) {
            throw ConfigurationException(getConfigRoot(), "property name is null",
                                         lineNumber);
        }

        if (propertyType == nullptr) {
            throw ConfigurationException(getConfigRoot(), "property type is null",
                                         lineNumber);
        }

        if (propertyValue == nullptr) {
            throw ConfigurationException(getConfigRoot(), "property value is null",
                                         lineNumber);
        }

        std::shared_ptr<PropertyBase> property = PropertyFactory::Create(
                propertyName, instanceName, className, processName, propertyType,
                propertyValue, "", false);

        property->setDataStorage(
                DataStorage(PropertyRepositoryType::FILE_REPOSITORY,
                            fileName + ":" + std::to_string(lineNumber)));

        properties.push_back(std::make_shared<PropertyEntry>(
                type_name<PropertyComponent>(), property));
    }
    return properties;
}
