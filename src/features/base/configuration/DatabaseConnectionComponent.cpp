#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"

#include <tinyxml2.h>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

DatabaseConnectionComponent::Shapes DatabaseConnectionComponent::shape{};

DatabaseConnectionComponent::DatabaseConnectionComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>> DatabaseConnectionComponent::parse(
    const std::string &content, const std::string &fileName,
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
    const char *name =
        propertyElement->Attribute(shape.CONNECTION_NAME.c_str());
    const char *connection =
        propertyElement->Attribute(shape.CONNECTION_CONNECTION.c_str());
    const char *password =
        propertyElement->Attribute(shape.CONNECTION_PASSWORD.c_str());
    const char *typeName =
        propertyElement->Attribute(shape.CONNECTION_TYPE.c_str());
    const char *userName =
        propertyElement->Attribute(shape.CONNECTION_USER_NAME.c_str());
    const char *portTemp =
        propertyElement->Attribute(shape.CONNECTION_PORT.c_str());
    const char *databaseName =
        propertyElement->Attribute(shape.CONNECTION_DATBASE_NAME.c_str());

    int32_t lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

    if (typeName == nullptr) {
      LOG_ERROR("type string at line {} is null", lineNumber);
      continue;
    }

    if (connection == nullptr) {
      connection = "";
    }

    db::ConnectionType type(typeName);
    int32_t port = -1;

    if (type == db::ConnectionType::UNDEFINED) {
      LOG_ERROR("type {} is not valid at line {}", typeName, lineNumber);
      continue;
    }

    if (name == nullptr) {
      LOG_ERROR("name string at line {} is null", lineNumber);
      continue;
    }

    if (portTemp != nullptr) {
      port = std::stoi(portTemp);
    }

    if (password == nullptr) {
      password = "";
    }

    if (userName == nullptr) {
      userName = "";
    }

    if (databaseName == nullptr) {
      databaseName = "";
    }

    databaseEntries.push_back(std::make_shared<DatabaseConnectionEntry>(
        type_name<DatabaseConnectionComponent>(), connection, userName, password, type,
        name, port, databaseName));
  }
  return databaseEntries;
}
