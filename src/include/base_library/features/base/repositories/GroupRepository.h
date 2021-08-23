#ifndef CPP_BASE_LIBRARY_GROUPREPOSITORY_H
#define CPP_BASE_LIBRARY_GROUPREPOSITORY_H

#include <memory>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/features/base/models/Group.h"

class GroupRepository {
 private:
  // Variables:
  // injections
  std::shared_ptr<ConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<ConnectionEntry> m_connectionEntry;
  // cache of groups
  std::vector<Group> m_groups;

  void initGroups();
 public:
  explicit GroupRepository(
      std::shared_ptr<ConnectionConfigurations> connectionConfigurations);
};

#endif  // CPP_BASE_LIBRARY_GROUPREPOSITORY_H
