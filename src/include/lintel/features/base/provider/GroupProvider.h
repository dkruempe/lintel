#ifndef LINTEL_GROUPPROVIDER_H
#define LINTEL_GROUPPROVIDER_H

#include "lintel/features/base/models/Group.h"

/**
 * interface for providing of virtual groups
 */
class GroupProvider {
private:
    std::vector<Group> m_groups;

public:
    GroupProvider() = default;

    /**
     * Add a virtual group to this provider.
     * @param group the group (must be virtual)
     * @throws std::runtime_error if the group is not virtual
     */
    void add(const Group &group) {
        if (!group.isVirtual()) {
            throw std::runtime_error("Group >" + group.getGroupName() +
                                     "< is not virtual");
        }
        m_groups.push_back(group);
    }

    virtual ~GroupProvider() = default;

    /** @return all provided virtual groups */
    const std::vector<Group> &provide() { return m_groups; }
};

#endif  // LINTEL_GROUPPROVIDER_H
