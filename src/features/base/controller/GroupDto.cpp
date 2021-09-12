#include "base_library/features/base/controller/GroupDto.h"

#include "base_library/core/services/LoggerService.h"
GroupDto::Shapes GroupDto::shape{};
GroupDto::GroupDto(const Group& group)
    : m_groupName(group.getGroupName()),
      m_groups(std::make_shared<GroupsDto>(group.getGroups())),
      m_isVirtual(group.isVirtual()) {}
void GroupDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartObject();
  // GROUP_NAME
  writer->String(shape.GROUP_NAME.c_str());
  writer->String(m_groupName.c_str());
  if (m_groups != nullptr) {
    // GROUPS
    writer->String(shape.GROUPS.c_str());
    m_groups->serialize(writer);
  }
  // VIRTUAL
  writer->String(shape.VIRTUAL.c_str());
  writer->Bool(m_isVirtual);
  writer->EndObject();
}
bool GroupDto::deserialize(const rapidjson::Value& obj) {
  bool success = true;
  if (obj.HasMember(shape.GROUP_NAME.c_str())) {
    m_groupName = obj[shape.GROUP_NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("deserialization of {} failed", shape.GROUP_NAME);
  }
  if (obj.HasMember(shape.VIRTUAL.c_str())) {
    m_isVirtual = obj[shape.VIRTUAL.c_str()].GetBool();
  } else {
    success = false;
    LOG_ERROR("deserialization of {} failed", shape.VIRTUAL);
  }
  if (obj.HasMember(shape.GROUPS.c_str())) {
    std::shared_ptr<GroupsDto> groupsDto = std::make_shared<GroupsDto>();
    groupsDto->deserialize(obj[shape.GROUPS.c_str()]);
    m_groups = std::move(groupsDto);
  }
  return success;
}
const std::string& GroupDto::getGroupName() const { return m_groupName; }
bool GroupDto::isVirtual() const { return m_isVirtual; }
std::vector<GroupDto> GroupDto::getSubGroups() const {
  if (m_groups == nullptr) {
    return {};
  }
  return m_groups->getGroups();
}
GroupsDto::GroupsDto(const std::vector<Group>& groups)
    : m_groups(init(groups)) {}
std::vector<GroupDto> GroupsDto::init(const std::vector<Group>& groups) {
  std::vector<GroupDto> groupDtos;
  groupDtos.reserve(groups.size());
  for (const auto& item : groups) {
    groupDtos.emplace_back(item);
  }
  return groupDtos;
}
void GroupsDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartArray();
  for (const auto& item : m_groups) {
    item.serialize(writer);
  }
  writer->EndArray();
}
bool GroupsDto::deserialize(const rapidjson::Value& obj) {
  if (!obj.IsArray()) {
    return false;
  }
  for (auto iter = obj.Begin(); iter != obj.End(); iter++) {
    GroupDto groupDto;
    groupDto.deserialize(*iter);
    m_groups.push_back(std::move(groupDto));
  }
  return true;
}
const std::vector<GroupDto>& GroupsDto::getGroups() const { return m_groups; }
