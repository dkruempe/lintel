#include "lintel/features/base/controller/GroupDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
const GroupDto::Shapes GroupDto::shape{};

GroupDto::GroupDto(const Group &group)
  : m_groupName(group.getGroupName()), m_groups(GroupsDto(group.getGroups()).getGroups()),
    m_isVirtual(group.isVirtual())
{}

void GroupDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // GROUP_NAME
  writer->String(shape.GROUP_NAME);
  writer->String(m_groupName.c_str());
  if (!m_groups.empty()) {
    // GROUPS
    writer->String(shape.GROUPS);
    GroupsDto dto(m_groups);
    dto.serialize(writer);
  }
  // VIRTUAL
  writer->String(shape.VIRTUAL);
  writer->Bool(m_isVirtual);
  writer->EndObject();
}

bool GroupDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, shape.GROUP_NAME, m_groupName)) {
    success = false;
    LOG_ERROR("deserialization of {} failed", shape.GROUP_NAME);
  }
  if (!json::getBool(obj, shape.VIRTUAL, m_isVirtual)) {
    success = false;
    LOG_ERROR("deserialization of {} failed", shape.VIRTUAL);
  }
  if (const auto *groups = json::member(obj, shape.GROUPS); groups != nullptr) {
    GroupsDto dto;
    dto.deserialize(*groups);
    m_groups = dto.getGroups();
  }
  return success;
}

const std::string &GroupDto::getGroupName() const { return m_groupName; }

bool GroupDto::isVirtual() const { return m_isVirtual; }

const std::vector<GroupDto> &GroupDto::getSubGroups() const { return m_groups; }

GroupsDto::GroupsDto(const std::vector<Group> &groups) : m_groups(init(groups)) {}

std::vector<GroupDto> GroupsDto::init(const std::vector<Group> &groups)
{
  std::vector<GroupDto> groupDtos;
  groupDtos.reserve(groups.size());
  std::transform(
    groups.begin(), groups.end(), std::back_inserter(groupDtos), [](const Group &group) { return GroupDto(group); });
  return groupDtos;
}

void GroupsDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &item : m_groups) { item.serialize(writer); }
  writer->EndArray();
}

bool GroupsDto::deserialize(const rapidjson::Value &obj)
{
  if (!obj.IsArray()) { return false; }
  for (const auto *iter = obj.Begin(); iter != obj.End(); iter++) {
    if (!iter->IsObject()) { continue; }
    GroupDto groupDto;
    groupDto.deserialize(*iter);
    m_groups.push_back(std::move(groupDto));
  }
  return true;
}

const std::vector<GroupDto> &GroupsDto::getGroups() const { return m_groups; }

GroupsDto::GroupsDto(std::vector<GroupDto> groups) : m_groups(std::move(groups)) {}
}// namespace lintel