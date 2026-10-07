#ifndef LINTEL_GROUPDTO_H
#define LINTEL_GROUPDTO_H

#include <memory>

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/models/Group.h"

class GroupsDto;

/** DTO representing a single user group */
class GroupDto : public JsonSerializable {
private:
    /** The group name */
    std::string m_groupName;
    /** Nested sub-groups */
    std::vector<GroupDto> m_groups;
    /** Whether this is a virtual group */
    bool m_isVirtual;

    /** JSON field name constants */
    static const struct Shapes {
        const char *const GROUP_NAME = "group_name";
        const char *const GROUPS = "groups";
        const char *const VIRTUAL = "virtual";
    } shape;

public:
    /** Construct from a Group model
     * @param group The source group */
    explicit GroupDto(const Group &group);

    /** Default constructor */
    GroupDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;

    /** Get the group name
     * @return The group name */
    [[nodiscard]] const std::string &getGroupName() const;

    /** Check if this is a virtual group
     * @return True if virtual */
    [[nodiscard]] bool isVirtual() const;

    /** Get the sub-groups
     * @return Vector of sub-group DTOs */
    [[nodiscard]] const std::vector<GroupDto> &getSubGroups() const;
};

/** DTO representing a collection of user groups */
class GroupsDto : public JsonSerializable {
private:
    /** The list of group DTOs */
    std::vector<GroupDto> m_groups;

    /** Initialize group DTOs from Group models
     * @param groups The source groups
     * @return Vector of group DTOs */
    static std::vector<GroupDto> init(const std::vector<Group> &groups);

public:
    /** Construct from Group models
     * @param groups The source groups */
    explicit GroupsDto(const std::vector<Group> &groups);

    /** Construct from existing GroupDto objects
     * @param groups The group DTOs */
    explicit GroupsDto(std::vector<GroupDto> groups);

    /** Default constructor */
    GroupsDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;

    /** Get the group DTOs
     * @return Vector of group DTOs */
    [[nodiscard]] const std::vector<GroupDto> &getGroups() const;
};

#endif  // LINTEL_GROUPDTO_H
