#ifndef CPP_BASE_LIBRARY_GROUPREPOSITORY_H
#define CPP_BASE_LIBRARY_GROUPREPOSITORY_H

#include <memory>
#include <optional>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/base/models/Group.h"
#include "base_library/core/models/ProcessName.h"

/**
 * Repository for CRUD operations on Group entities stored in the database.
 * Manages group hierarchy including parent-child group relationships.
 */
class GroupRepository : public PersistableBean {
private:
    // Variables:
    // injections
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    // cache of groups
    std::vector<Group> m_groups;
    std::map<std::string, Group> m_groupMap;

    /** Initialize the group cache from the database. */
    void initGroups();

public:
    /**
     * Constructor.
     * @param connectionConfigurations database connection configuration
     */
    explicit GroupRepository(std::shared_ptr<DatabaseConnectionConfigurations>
                             connectionConfigurations);

    virtual ~GroupRepository() = default;

    /**
     * Find a group by name.
     * @param groupName group name
     * @return the group if found
     */
    std::optional<Group> of(const std::string &groupName);

    /** @return all groups */
    std::vector<Group> allOf();

    /**
     * @param groupName group name filter
     * @return groups matching the given name
     */
    std::vector<Group> allOf(const std::string &groupName);

    /**
     * @param isVirtualGroup filter by virtual status
     * @return groups matching the given virtual flag
     */
    std::vector<Group> allOf(bool isVirtualGroup);

    /**
     * @param groupName group name filter
     * @param isVirtualGroup filter by virtual status
     * @return groups matching both filters
     */
    std::vector<Group> allOf(const std::string &groupName, bool isVirtualGroup);

    /**
     * Create a new group.
     * @param group group to create
     */
    void createOf(const Group &group);

    /**
     * Delete a group.
     * @param group group to delete
     */
    void deleteOf(const Group &group);

    /**
     * Add a sub-group to an existing group.
     * @param group parent group
     * @param add sub-group to add
     */
    void addGroupOf(const Group &group, const Group &add);

    /**
     * Remove a sub-group from an existing group.
     * @param group parent group
     * @param remove sub-group to remove
     */
    void removeGroupOf(const Group &group, const Group &remove);

    /** Initialize the repository (load groups from database). */
    void onAwake() override;
};

#endif  // CPP_BASE_LIBRARY_GROUPREPOSITORY_H
