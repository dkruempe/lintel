#include "base_library/features/base/models/Group.h"

#include <utility>
Group::Group(std::string groupName, std::vector<Group> groups, bool isVirtual)
    : m_groupName(std::move(groupName)),
      m_groups(std::move(groups)),
      m_isVirtual(isVirtual) {}
const std::string& Group::getGroupName() const { return m_groupName; }
const std::vector<Group>& Group::getGroups() const { return m_groups; }
bool Group::isVirtual() const { return m_isVirtual; }
std::ostream& operator<<(std::ostream& os, const Group& group) {
  os << "Group{"
     << "group_name: " << group.m_groupName
     << ", isVirtual:" << group.m_isVirtual
     << ", m_groups:" << group.m_groups.size() << "}";
  return os;
}
