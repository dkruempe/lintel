#include "base_library/features/base/configuration/ConnectionComponent.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/TypeName.h"

#include <tinyxml2.h>

ConnectionComponent::Shapes ConnectionComponent::shape{};

ConnectionComponent::ConnectionComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>>
ConnectionComponent::parse(const std::string &content,
                           const std::string &fileName,
                           const int32_t lineOffset) {
  std::vector<std::shared_ptr<Entry>> databaseEntries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());

  tinyxml2::XMLElement *rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return databaseEntries;
  }

  for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement();
       propertyElement != nullptr;
       propertyElement = propertyElement->NextSiblingElement()) {
    if (std::strcmp(propertyElement->Name(), shape.DATABASE_ROOT.c_str()) !=
        0) {
      continue;
    }
    const char *connection =
        propertyElement->Attribute(shape.CONNECTION_CONNECTION.c_str());
    const char *password =
        propertyElement->Attribute(shape.CONNECTION_PASSWORD.c_str());
    const char *type =
        propertyElement->Attribute(shape.CONNECTION_TYPE.c_str());
    const char *userName =
        propertyElement->Attribute(shape.CONNECTION_USER_NAME.c_str());

    int32_t lineNumber = propertyElement->GetLineNum();

    if (connection == nullptr) {
      LOG_ERROR("connection string at line {} is null", lineNumber);
      continue;
    }

    if (password == nullptr) {
      LOG_ERROR("passwoord string at line {} is null", lineNumber);
      continue;
    }

    if (type == nullptr) {
      LOG_ERROR("type string at line {} is null", lineNumber);
      continue;
    }

    if (userName == nullptr) {
      LOG_ERROR("userName string at line {} is null", lineNumber);
      continue;
    }
    databaseEntries.push_back(std::make_shared<ConnectionEntry>(
        type_name<ConnectionComponent>(), connection, userName, password,
        type));
  }
  return databaseEntries;
}
