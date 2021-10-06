#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"

#include <algorithm>
#include <map>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"

DatabaseConnectionConfigurations::DatabaseConnectionConfigurations(
    const std::shared_ptr<Configuration> &configuration)
    : m_connections(build(
          configuration->configurationOf<DatabaseConnectionComponent>())) {}

std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>>
DatabaseConnectionConfigurations::build(
    std::vector<std::shared_ptr<Entry>> entries) {
  std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> connections;
  std::transform(
      entries.begin(), entries.end(),
      std::inserter(connections, connections.begin()),
      [](std::shared_ptr<Entry> &entry)
          -> std::pair<std::string, std::shared_ptr<DatabaseConnectionEntry>> {
        std::shared_ptr<DatabaseConnectionEntry> connectionEntry =
            std::static_pointer_cast<DatabaseConnectionEntry>(entry);
        return {connectionEntry->getName(), connectionEntry};
      });
  return connections;
}
std::shared_ptr<DatabaseConnectionEntry> DatabaseConnectionConfigurations::of(
    const std::string &connectionName) {
  try {
    return m_connections.at(connectionName);
  } catch (std::out_of_range &e) {
    throw db::SQLException("Connection with " + connectionName + " not found");
  }
}
std::vector<std::shared_ptr<DatabaseConnectionEntry>>
DatabaseConnectionConfigurations::allof() {
  std::vector<std::shared_ptr<DatabaseConnectionEntry>> connections;
  for (auto &[name, connection] : m_connections) {
    connections.push_back(connection);
  }
  return connections;
}
