#ifndef CPP_BASE_LIBRARY_USERGROUP_H
#define CPP_BASE_LIBRARY_USERGROUP_H

#include <set>
#include <string>

#include "base_library/core/models/JsonSerializable.h"

class UserGroupDto : public JsonSerializable {
private:
    std::set<std::string> m_groupAdds;
    std::set<std::string> m_groupRemoves;
    std::string m_userName;

    static struct Shapes {
        const std::string USER_NAME = "user_name";
        const std::string GROUPS_ADD = "groups_add";
        const std::string GROUPS_REMOVE = "groups_remove";
    } shape;

public:
    UserGroupDto() = default;

    UserGroupDto(std::set<std::string> groupAdds,
                 std::set<std::string> groupRemoves, std::string userName);

    [[nodiscard]] const std::string &getUserName();

    [[nodiscard]] const std::set<std::string> &getGroupAdds();

    [[nodiscard]] const std::set<std::string> &getGroupRemoves();

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERGROUP_H
