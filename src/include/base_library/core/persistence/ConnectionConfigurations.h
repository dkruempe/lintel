#ifndef CPP_BASE_LIBRARY_CONNECTIONCONFIGURATIONS_H
#define CPP_BASE_LIBRARY_CONNECTIONCONFIGURATIONS_H

#include <map>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"

class ConnectionConfigurations {
 private:
  std::map<std::string, std::shared_ptr<ConnectionEntry>> m_connections;

  static std::map<std::string, std::shared_ptr<ConnectionEntry>> build(
      std::vector<std::shared_ptr<Entry>> entries);

 public:
  explicit ConnectionConfigurations(
      const std::shared_ptr<Configuration> &configuration);

  [[nodiscard]] std::shared_ptr<ConnectionEntry> of(
      const std::string &connectionName);

  [[nodiscard]] std::vector<std::shared_ptr<ConnectionEntry>> allof();
};

#endif  // CPP_BASE_LIBRARY_CONNECTIONCONFIGURATIONS_H
