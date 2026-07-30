#ifndef CPP_BASE_LIBRARY_GROUP_H
#define CPP_BASE_LIBRARY_GROUP_H

#include <ostream>
#include <string>
#include <vector>

/**
 * Way to control access rights.
 * The following rules are used to assign groups to different groups
 * 1. You cannot assign the same group to groups (no circles)
 * 2. You can only connect with no listed groups (only base groups for
 * collecting groups)
 * 3. Collecting groups are used for something like Admin groups
 *
 * Example:
 * For example you can use non assign able groups to define the access rights
 * for new parts of the software. Something like a _PropertyGroupMember
 * or _PropertyGroupAdmin. Those groups are used in the PropertyController
 * itself to define check the access rights itself.
 *
 * The user is than not assigned to the mentioned groups. There is something
 * like Administrator or Member which contains those virtual groups.
 *
 * Those virtual groups are collected via an Abstract class interface and are
 * not persisted in the database.
 *
 * An separate service will check the validation of the users. This validation
 * will be checked during the login process and not periodically, bc. of the
 * possibility of a high count of users, but there will be the possibility to
 * trigger those checks via the cli itself.
 */
class Group {
private:
    std::string m_groupName;
    std::vector<Group> m_groups;
    bool m_isVirtual;

public:
    /**
     * Constructor.
     * @param groupName name of the group
     * @param groups sub-groups of this group
     * @param isVirtual whether this group is virtual (not persisted)
     */
    Group(std::string groupName, std::vector<Group> groups, bool isVirtual);

    /** @return group name */
    [[nodiscard]] const std::string &getGroupName() const;

    /** @return sub-groups of this group */
    [[nodiscard]] const std::vector<Group> &getGroups() const;

    /** @return true if this group is virtual */
    [[nodiscard]] bool isVirtual() const;

    /** @return stream representation of the group */
    friend std::ostream &operator<<(std::ostream &os, const Group &group);

    /** @return true if groups are equal */
    bool operator==(const Group &rhs) const;

    /** @return true if groups are not equal */
    bool operator!=(const Group &rhs) const;

    /** @return true if this group is less than rhs */
    bool operator<(const Group &rhs) const;

    /** @return true if this group is greater than rhs */
    bool operator>(const Group &rhs) const;

    /** @return true if this group is less than or equal to rhs */
    bool operator<=(const Group &rhs) const;

    /** @return true if this group is greater than or equal to rhs */
    bool operator>=(const Group &rhs) const;
};

#endif  // CPP_BASE_LIBRARY_GROUP_H
