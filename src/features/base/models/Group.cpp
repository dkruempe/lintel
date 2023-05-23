#include "base_library/features/base/models/Group.h"

#include <utility>

Group::Group(std::string groupName, std::vector<Group> groups, bool isVirtual)
        : m_groupName(std::move(groupName)),
          m_groups(std::move(groups)),
          m_isVirtual(isVirtual) {}

const std::string &Group::getGroupName() const { return m_groupName; }

const std::vector<Group> &Group::getGroups() const { return m_groups; }

bool Group::isVirtual() const { return m_isVirtual; }

std::ostream &operator<<(std::ostream &os, const Group &group) {
    os << "Group{"
       << "group_name: " << group.m_groupName
       << ", isVirtual:" << group.m_isVirtual;
    if (!group.m_groups.empty()) {
        os << ", m_groups: {";
        for (std::size_t i = 0; i < group.m_groups.size(); i++) {
            Group memberGroup = group.m_groups[i];
            os << memberGroup;
            if (i != memberGroup.m_groups.size() - 1) {
                os << ", ";
            }
        }
        os << "}";
    }
    os << "}";
    return os;
}

bool Group::operator==(const Group &rhs) const {
    return m_groupName == rhs.m_groupName && m_groups == rhs.m_groups &&
           m_isVirtual == rhs.m_isVirtual;
}

bool Group::operator!=(const Group &rhs) const { return !(rhs == *this); }

bool Group::operator<(const Group &rhs) const {
    if (m_groupName < rhs.m_groupName) return true;
    if (rhs.m_groupName < m_groupName) return false;
    if (m_groups < rhs.m_groups) return true;
    if (rhs.m_groups < m_groups) return false;
    return m_isVirtual < rhs.m_isVirtual;
}

bool Group::operator>(const Group &rhs) const { return rhs < *this; }

bool Group::operator<=(const Group &rhs) const { return !(rhs < *this); }

bool Group::operator>=(const Group &rhs) const { return !(*this < rhs); }
