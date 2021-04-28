#include <algorithm>
#include <map>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/features/base/configuration/ConnectionComponent.h"

ConnectionConfigurations::ConnectionConfigurations(
    const std::shared_ptr<Configuration> &configuration)
    : connections(
          build(configuration->configurationOf<ConnectionComponent>())) {}

std::map<std::string, std::shared_ptr<ConnectionEntry>>
ConnectionConfigurations::build(std::vector<std::shared_ptr<Entry>> entries) {
  std::map<std::string, std::shared_ptr<ConnectionEntry>> connections;
  std::transform(
      entries.begin(), entries.end(),
      std::inserter(connections, connections.begin()),
      [](std::shared_ptr<Entry> &entry)
          -> std::pair<std::string, std::shared_ptr<ConnectionEntry>> {
        std::shared_ptr<ConnectionEntry> connectionEntry =
            std::static_pointer_cast<ConnectionEntry>(entry);
        return {connectionEntry->getName(), connectionEntry};
      });
  return connections;
}
std::shared_ptr<ConnectionEntry> ConnectionConfigurations::of(
    const std::string &connectionName) {
  try {
    return connections.at(connectionName);
  } catch (std::out_of_range &e) {
    throw db::SQLException("Connection with " + connectionName + " not found");
  }
}
