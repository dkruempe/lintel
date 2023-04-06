#ifndef CPP_BASE_LIBRARY_HISTORYREPOSITORY_H
#define CPP_BASE_LIBRARY_HISTORYREPOSITORY_H

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/models/HistoryEntry.h"

class HistoryRepository {
 private:
  // injections
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

 public:
  // select methods
  [[nodiscard]] std::vector<HistoryEntry> allOf() const;
  [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &label) const;
  [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &processName,
                                  const std::string &serviceName) const;
  [[nodiscard]] std::vector<HistoryEntry> allOfProcess(const std::string &processName) const;
  [[nodiscard]] std::vector<HistoryEntry> allOfService(const std::string &serviceName) const;

  // insert
  void insertOf(const std::vector<HistoryEntry> &entries);

  // delete
  void cleanAllOlderThan(date::sys_time<std::chrono::microseconds> timestamp);

  explicit HistoryRepository(std::shared_ptr<DatabaseConnectionConfigurations>
                        databaseConnectionConfigurations);
};

#endif  // CPP_BASE_LIBRARY_HISTORYREPOSITORY_H
