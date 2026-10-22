#include "lintel/core/configuration/DatabaseConnectionComponent.h"

#include <lintel/core/persistence/Connection.h>
#include <tinyxml2.h>

#include <iostream>
#include <memory>

#include "lintel/core/configuration/ConfigurationException.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/utils/TypeName.h"

namespace lintel {
const DatabaseConnectionComponent::Shapes DatabaseConnectionComponent::shape{};

DatabaseConnectionComponent::DatabaseConnectionComponent(
  std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
  : Component(shape.CONFIG_ROOT), m_environmentConfiguration(std::move(environmentConfiguration))
{}

/**
 * Parse one `<database>`/`<connection>` element into a DatabaseConnectionEntry.
 *
 * Required attributes are validated in the order the entry needs them, so the
 * first error reported refers to the first missing or invalid value. SQLite
 * connections additionally resolve a leading `~` against the configured home
 * directory and reject `poolSize > 1` unless multiple connections are allowed
 * explicitly.
 *
 * @param propertyElement XML element carrying the connection attributes
 * @param lineOffset      offset added to the XML line number for diagnostics
 * @param configRoot      configuration root used in exception messages
 * @param envConfig       environment configuration resolving the home directory
 * @param foundConnectionWithDefault set to true when this entry is the default;
 *        a second default entry is rejected
 * @return the parsed entry
 * @throws ConfigurationException when a required attribute is missing, a value
 *         is invalid, or a second default connection is declared
 */
std::shared_ptr<Entry> DatabaseConnectionComponent::parseDatabaseEntry(tinyxml2::XMLElement *propertyElement,
  int32_t lineOffset,
  const std::string &configRoot,
  const std::shared_ptr<EnvironmentConfiguration> &envConfig,
  bool &foundConnectionWithDefault)
{
  // Attributes are optional at this point; each one is validated at the place
  // where it is consumed below.
  const char *name = propertyElement->Attribute(shape.CONNECTION_NAME);
  const char *connection = propertyElement->Attribute(shape.CONNECTION_CONNECTION);
  const char *password = propertyElement->Attribute(shape.CONNECTION_PASSWORD);
  const char *typeName = propertyElement->Attribute(shape.CONNECTION_TYPE);
  const char *userName = propertyElement->Attribute(shape.CONNECTION_USER_NAME);
  const char *portTemp = propertyElement->Attribute(shape.CONNECTION_PORT);
  const char *databaseName = propertyElement->Attribute(shape.CONNECTION_DATBASE_NAME);
  const char *defaultName = propertyElement->Attribute(shape.CONNECTION_DEFAULT);
  const char *poolSizeTemp = propertyElement->Attribute(shape.CONNECTION_POOL_SIZE);
  const char *poolAcquireTimeoutTemp = propertyElement->Attribute(shape.CONNECTION_POOL_ACQUIRE_TIMEOUT_MS);
  const char *allowMultipleSqlite = propertyElement->Attribute(shape.CONNECTION_ALLOW_MULTIPLE_SQLITE_CONNECTIONS);

  int32_t lineNumber = propertyElement->GetLineNum() + lineOffset - 1;

  // The type decides how the remaining attributes are interpreted, so it must
  // be present before anything else is checked.
  if (typeName == nullptr) { throw ConfigurationException(configRoot, "type string is null", lineNumber); }
  std::string connectionString = connection != nullptr ? connection : "";

  // At most one connection may be marked as default.
  bool isDefault = false;
  if (defaultName != nullptr) {
    isDefault = std::strcmp(defaultName, "true") == 0 || std::strcmp(defaultName, "t") == 0;
    if (foundConnectionWithDefault) {
      throw ConfigurationException(configRoot, "default can only once set to true", lineNumber);
    }
    foundConnectionWithDefault = true;
  }

  db::ConnectionType type(typeName);
  int32_t port = -1;

  if (type == db::ConnectionType::UNDEFINED) {
    throw ConfigurationException(configRoot, "type " + std::string(typeName) + " is not valid", lineNumber);
  }
  // SQLite stores the database as a file, so a leading `~` has to be expanded
  // relative to the environment's home directory instead of the process CWD.
  if (type == db::ConnectionType::SQLite) {
    std::string tmpPath = connection;
    if (!tmpPath.empty() && tmpPath[0] == '~') {
      std::string restPath(tmpPath.begin() + 1, tmpPath.end());
      tmpPath = envConfig->of(EnvironmentConfiguration::Home);
      tmpPath += '/';
      tmpPath += restPath;
    }
    connectionString = tmpPath;
  }

  if (name == nullptr) { throw ConfigurationException(configRoot, "name string is null", lineNumber); }

  // Numeric attributes are parsed only when present; -1 means "not set" and is
  // passed through to the entry as such.
  if (portTemp != nullptr) { port = std::stoi(portTemp); }

  // Missing connection details default to the empty string instead of failing:
  // SQLite needs none of them, and the server backends validate on connect.
  if (password == nullptr) { password = ""; }

  if (userName == nullptr) { userName = ""; }

  if (databaseName == nullptr) { databaseName = ""; }

  // Pool settings are optional; a present value must not be negative.
  int32_t poolSize = -1;
  if (poolSizeTemp != nullptr) {
    poolSize = std::stoi(poolSizeTemp);
    if (poolSize < 0) {
      throw ConfigurationException(
        configRoot, std::string(shape.CONNECTION_POOL_SIZE) + " must not be negative", lineNumber);
    }
  }

  int32_t poolAcquireTimeoutMs = -1;
  if (poolAcquireTimeoutTemp != nullptr) {
    poolAcquireTimeoutMs = std::stoi(poolAcquireTimeoutTemp);
    if (poolAcquireTimeoutMs < 0) {
      throw ConfigurationException(
        configRoot, std::string(shape.CONNECTION_POOL_ACQUIRE_TIMEOUT_MS) + " must not be negative", lineNumber);
    }
  }

  // SQLite serializes writers through a single file lock; a larger pool only
  // helps when the operator opted in explicitly.
  if (type == db::ConnectionType::SQLite) {
    const int32_t effectivePoolSize = poolSize >= 0 ? poolSize : DatabaseConnectionEntry::kDefaultSqlitePoolSize;
    const bool allowMultipleConnections =
      allowMultipleSqlite != nullptr
      && (std::strcmp(allowMultipleSqlite, "true") == 0 || std::strcmp(allowMultipleSqlite, "t") == 0);
    if (effectivePoolSize > 1 && !allowMultipleConnections) {
      throw ConfigurationException(configRoot,
        "poolSize > 1 for SQLite connection '" + std::string(name) + "' risks WAL/file-lock contention; "
        "set allowMultipleSqliteConnections=\"true\" to override",
        lineNumber);
    }
  }

  return std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
    connectionString,
    userName,
    password,
    type,
    name,
    port,
    databaseName,
    isDefault,
    poolSize,
    poolAcquireTimeoutMs);
}

std::vector<std::shared_ptr<Entry>>
  DatabaseConnectionComponent::parse(const std::string &content, const int32_t lineOffset, const std::string &fileName)
{
  std::vector<std::shared_ptr<Entry>> databaseEntries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());

  tinyxml2::XMLElement *rootNode = document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) { return databaseEntries; }

  bool foundConnectionWithDefault = false;
  for (tinyxml2::XMLElement *propertyElement = rootNode->FirstChildElement(); propertyElement != nullptr;
       propertyElement = propertyElement->NextSiblingElement()) {
    if (std::strcmp(propertyElement->Name(), shape.DATABASE_ROOT) != 0) { continue; }

    auto entry = parseDatabaseEntry(
      propertyElement, lineOffset, getConfigRoot(), m_environmentConfiguration, foundConnectionWithDefault);
    databaseEntries.push_back(entry);
  }
  if (!foundConnectionWithDefault) {
    throw ConfigurationException(getConfigRoot(), "please configure a default database", lineOffset);
  }
  return databaseEntries;
}
}// namespace lintel
