#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H

#include <map>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

class DatabaseConnectionConfigurations {
 private:
  std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> m_connections;
  std::shared_ptr<DatabaseConnectionEntry> m_defaultConnection;

  static std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> build(
      std::vector<std::shared_ptr<Entry>> entries);

  static std::shared_ptr<DatabaseConnectionEntry> findDefault(
      const std::vector<std::shared_ptr<Entry>> &entries);

 public:
  explicit DatabaseConnectionConfigurations(
      const std::shared_ptr<Configuration> &configuration);

  [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> ofDefault();

  [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> of(
      const std::string &connectionName);

  [[nodiscard]] std::vector<std::shared_ptr<DatabaseConnectionEntry>> allOf();
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
