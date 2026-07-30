#ifndef CPP_BASE_LIBRARY_USERGROUP_H
#define CPP_BASE_LIBRARY_USERGROUP_H

#include <set>
#include <string>

#include "base_library/core/models/JsonSerializable.h"

/** DTO for modifying a user's group memberships */
class UserGroupDto : public JsonSerializable {
private:
    /** Groups to add */
    std::set<std::string> m_groupAdds;
    /** Groups to remove */
    std::set<std::string> m_groupRemoves;
    /** The user name */
    std::string m_userName;

    /** JSON field name constants */
    static struct Shapes {
        const char* const USER_NAME = "user_name";
        const char* const GROUPS_ADD = "groups_add";
        const char* const GROUPS_REMOVE = "groups_remove";
    } shape;

public:
    /** Default constructor */
    UserGroupDto() = default;

    /** Construct a user group modification DTO
     * @param groupAdds Set of groups to add
     * @param groupRemoves Set of groups to remove
     * @param userName The target user name */
    UserGroupDto(std::set<std::string> groupAdds,
                 std::set<std::string> groupRemoves, std::string userName);

    /** Get the user name
     * @return The user name */
    [[nodiscard]] const std::string &getUserName();

    /** Get the groups to add
     * @return Set of group names */
    [[nodiscard]] const std::set<std::string> &getGroupAdds();

    /** Get the groups to remove
     * @return Set of group names */
    [[nodiscard]] const std::set<std::string> &getGroupRemoves();

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERGROUP_H
