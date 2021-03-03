#include "base_library/configuration/PropertyComponent.h"
#include "base_library/configuration/PropertyEntry.h"
#include "base_library/factories/PropertyFactory.h"
#include "base_library/models/PropertyRepositoryType.h"
#include "base_library/services/LoggerService.h"
#include "base_library/utils/TypeName.h"

PropertyComponent::Shapes PropertyComponent::shape{};

PropertyComponent::PropertyComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>>
PropertyComponent::parse(const std::string &content,
                         const std::string &fileName,
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

    int32_t lineNumber = propertyElement->GetLineNum();

    if (processName == nullptr) {
      LOG_ERROR("process name at line {} is null", lineNumber);
      continue;
    }

    if (className == nullptr) {
      LOG_ERROR("class name at line {} is null", lineNumber);
      continue;
    }

    if (instanceName == nullptr) {
      LOG_ERROR("instance name at line {} is null", lineNumber);
      continue;
    }

    if (propertyName == nullptr) {
      LOG_ERROR("property name at line {} is null", lineNumber);
      continue;
    }

    if (propertyType == nullptr) {
      LOG_ERROR("property type at line {} is null", lineNumber);
      continue;
    }

    if (propertyValue == nullptr) {
      LOG_ERROR("property value at line {} is null", lineNumber);
      continue;
    }

    std::shared_ptr<PropertyBase> property = PropertyFactory::Create(
        propertyName, instanceName, className, processName, propertyType,
        propertyValue, "", false);

    property->setDataStorage(
        DataStorage(PropertyRepositoryType::FILE_REPOSITORY,
                    fileName + ":" + std::to_string(lineNumber + lineOffset - 1)));

    properties.push_back(std::make_shared<PropertyEntry>(
        type_name<PropertyComponent>(), property));
  }
  return properties;
}
