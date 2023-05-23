#ifndef CPP_BASE_LIBRARY_GROUPREPOSITORY_H
#define CPP_BASE_LIBRARY_GROUPREPOSITORY_H

#include <memory>
#include <optional>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/base/models/Group.h"
#include "base_library/features/base/models/ProcessName.h"

class GroupRepository : public PersistableBean {
private:
    // Variables:
    // injections
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    // cache of groups
    std::vector<Group> m_groups;
    std::map<std::string, Group> m_groupMap;

    void initGroups();

public:
    explicit GroupRepository(std::shared_ptr<DatabaseConnectionConfigurations>
                             connectionConfigurations);

    virtual ~GroupRepository() = default;

    std::optional<Group> of(const std::string &groupName);

    std::vector<Group> allOf();

    std::vector<Group> allOf(const std::string &groupName);

    std::vector<Group> allOf(bool isVirtualGroup);

    std::vector<Group> allOf(const std::string &groupName, bool isVirtualGroup);

    void createOf(const Group &group);

    void deleteOf(const Group &group);

    void addGroupOf(const Group &group, const Group &add);

    void removeGroupOf(const Group &group, const Group &remove);

    void onAwake() override;
};

#endif  // CPP_BASE_LIBRARY_GROUPREPOSITORY_H
