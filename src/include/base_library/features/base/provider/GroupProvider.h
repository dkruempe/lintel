#ifndef CPP_BASE_LIBRARY_GROUPPROVIDER_H
#define CPP_BASE_LIBRARY_GROUPPROVIDER_H

#include "base_library/features/base/models/Group.h"

/**
 * interface for providing of virtual groups
 */
class GroupProvider {
 private:
  std::vector<Group> m_groups;

 public:
  GroupProvider() = default;

  void add(const Group& group) {
    if (!group.isVirtual()) {
      throw std::runtime_error("Group >" + group.getGroupName() +
                               "< is not virtual");
    }
    m_groups.push_back(group);
  }

  virtual ~GroupProvider() = default;
  const std::vector<Group>& provide() { return m_groups; }
};

#endif  // CPP_BASE_LIBRARY_GROUPPROVIDER_H
