#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"

#include <base_library/core/persistence/Connection.h>
#include <tinyxml2.h>

#include <iostream>
#include <memory>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/ConfigurationException.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

DatabaseConnectionComponent::Shapes DatabaseConnectionComponent::shape{};

DatabaseConnectionComponent::DatabaseConnectionComponent(
    std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
    : Component(shape.CONFIG_ROOT),
      m_environmentConfiguration(std::move(environmentConfiguration)) {}

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

  bool foundConnectionWithDefault = false;
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
    const char *defaultName =
        propertyElement->Attribute(shape.CONNECTION_DEFAULT.c_str());

    int32_t lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

    if (typeName == nullptr) {
      throw ConfigurationException(getConfigRoot(), "type string is null",
                                   lineNumber);
    }
    std::string connectionString = connection != nullptr ? connection : "";

    bool isDefault = false;
    if (defaultName != nullptr) {
      isDefault = std::strcmp(defaultName, "true") == 0 ||
                  std::strcmp(defaultName, "t") == 0;
      if (foundConnectionWithDefault) {
        throw ConfigurationException(
            getConfigRoot(), "default can only once set to true", lineNumber);
      }
      foundConnectionWithDefault = true;
    }

    db::ConnectionType type(typeName);
    int32_t port = -1;

    if (type == db::ConnectionType::UNDEFINED) {
      throw ConfigurationException(
          getConfigRoot(), "type " + std::string(typeName) + " is not valid",
          lineNumber);
    }
    if (type == db::ConnectionType::SQLite) {
      std::string tmpPath = connection;
      if (tmpPath[0] == '~') {
        std::string restPath(tmpPath.begin() + 1, tmpPath.end());
        tmpPath =
            m_environmentConfiguration->of(EnvironmentConfiguration::Home);
        tmpPath += '/';
        tmpPath += restPath;
      }
      connectionString = tmpPath.c_str();
    }

    if (name == nullptr) {
      throw ConfigurationException(getConfigRoot(), "name string is null",
                                   lineNumber);
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
        type_name<DatabaseConnectionComponent>(), connectionString, userName,
        password, type, name, port, databaseName, isDefault));
  }
  if (!foundConnectionWithDefault) {
    throw ConfigurationException(
        getConfigRoot(), "please configure a default database", lineOffset);
  }
  return databaseEntries;
}
