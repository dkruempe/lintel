#ifndef CPP_BASE_LIBRARY_GROUPDTO_H
#define CPP_BASE_LIBRARY_GROUPDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/Group.h"
#include <memory>

class GroupsDto;

class GroupDto : public JsonSerializable {
 private:
  std::string m_groupName;
  std::shared_ptr<GroupsDto> m_groups;
  bool m_isVirtual;

  static struct Shapes {
    const std::string GROUP_NAME = "group_name";
    const std::string GROUPS = "groups";
    const std::string VIRTUAL = "virtual";
  } shape;

 public:
  explicit GroupDto(const Group &group);
  GroupDto() = default;
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  bool deserialize(const rapidjson::Value &obj) override;
  [[nodiscard]] const std::string &getGroupName() const;
  [[nodiscard]] bool isVirtual() const;
  std::vector<GroupDto> getSubGroups() const;
};

class GroupsDto : public JsonSerializable {
 private:
  std::vector<GroupDto> m_groups;

  static std::vector<GroupDto> init(const std::vector<Group> &groups);

 public:
  explicit GroupsDto(const std::vector<Group> &groups);
  GroupsDto() = default;
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  bool deserialize(const rapidjson::Value &obj) override;
  [[nodiscard]] const std::vector<GroupDto> &getGroups() const;
};

#endif  // CPP_BASE_LIBRARY_GROUPDTO_H
