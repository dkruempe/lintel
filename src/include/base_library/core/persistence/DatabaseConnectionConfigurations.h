#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H

#include <map>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

class DatabaseConnectionConfigurations {
 private:
  std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> m_connections;

  static std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> build(
      std::vector<std::shared_ptr<Entry>> entries);

 public:
  explicit DatabaseConnectionConfigurations(
      const std::shared_ptr<Configuration> &configuration);

  [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> of(
      const std::string &connectionName);

  [[nodiscard]] std::vector<std::shared_ptr<DatabaseConnectionEntry>> allof();
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
