#include "base_library/features/property/configuration/PropertyRepositoryComponent.h"

#include <base_library/features/property/configuration/PropertyRepositoryEntry.h>

#include <memory>

#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/ConfigurationException.h"
#include "base_library/features/property/factories/PropertyFactory.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

PropertyRepositoryComponent::Shapes PropertyRepositoryComponent::shape{};

PropertyRepositoryComponent::PropertyRepositoryComponent()
    : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>> PropertyRepositoryComponent::parse(
    const std::string &content, const std::string &fileName,
    const int32_t lineOffset) {
  std::vector<std::shared_ptr<Entry>> tmp;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());

  tinyxml2::XMLElement *rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return tmp;
  }

  for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement();
       propertyElement != nullptr;
       propertyElement = propertyElement->NextSiblingElement()) {
    if (std::strcmp(propertyElement->Name(),
                    shape.PROPERTY_REPOSITORY_ROOT.c_str()) != 0) {
      continue;
    }
    const char *typeStr =
        propertyElement->Attribute(shape.PROPERTY_REPOSITORY_TYPE.c_str());
    const char *isShadowStr =
        propertyElement->Attribute(shape.PROPERTY_REPOSITORY_SHADOW.c_str());
    const char *isMutableStr =
        propertyElement->Attribute(shape.PROPERTY_REPOSITORY_MUTABLE.c_str());

    int32_t lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

    if (typeStr == nullptr) {
      throw ConfigurationException(getConfigRoot(), "type is null", lineNumber);
    }

    if (isShadowStr == nullptr) {
      throw ConfigurationException(getConfigRoot(), "shadow is null",
                                   lineNumber);
    }

    if (isMutableStr == nullptr) {
      throw ConfigurationException(getConfigRoot(), "mutable is null",
                                   lineNumber);
    }

    PropertyRepositoryType type(typeStr);
    if (type == PropertyRepositoryType::DEFAULT ||
        type == PropertyRepositoryType::UNDEFINED) {
      throw ConfigurationException(getConfigRoot(), "invalid type", lineNumber);
    }

    bool isShadow = std::strcmp(isShadowStr, "true") == 0;
    bool isMutable = std::strcmp(isMutableStr, "true") == 0;

    if (type == PropertyRepositoryType::FILE_REPOSITORY &&
        (isShadow || isMutable)) {
      throw ConfigurationException(
          getConfigRoot(), "file repository cannot be an shadow or mutable",
          lineNumber);
    }

    if ((type == PropertyRepositoryType::SHM_REPOSITORY ||
         type == PropertyRepositoryType::DATABASE_REPOSITORY) &&
        isShadow && !isMutable) {
      throw ConfigurationException(getConfigRoot(),
                                   "can only be shadow if mutable", lineNumber);
    }
    std::shared_ptr<PropertyRepositoryEntry> propertyRepositoryEntry =
        std::make_shared<PropertyRepositoryEntry>(
            type_name<PropertyRepositoryComponent>(), type, isMutable,
            isShadow);
    tmp.push_back(propertyRepositoryEntry);
  }
  return tmp;
}
