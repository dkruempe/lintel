#include "lintel/features/base/controller/UserGroupDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
UserGroupDto::Shapes UserGroupDto::shape{};

UserGroupDto::UserGroupDto(std::set<std::string> groupAdds, std::set<std::string> groupRemoves, std::string userName)
  : m_groupAdds(std::move(groupAdds)), m_groupRemoves(std::move(groupRemoves)), m_userName(std::move(userName))
{}

const std::string &UserGroupDto::getUserName() { return m_userName; }

const std::set<std::string> &UserGroupDto::getGroupAdds() { return m_groupAdds; }

const std::set<std::string> &UserGroupDto::getGroupRemoves() { return m_groupRemoves; }

void UserGroupDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  writer->String(shape.USER_NAME);
  writer->String(m_userName.c_str());
  writer->String(shape.GROUPS_ADD);
  writer->StartArray();
  for (const auto &item : m_groupAdds) { writer->String(item.c_str()); }
  writer->EndArray();
  writer->String(shape.GROUPS_REMOVE);
  writer->StartArray();
  for (const auto &item : m_groupRemoves) { writer->String(item.c_str()); }
  writer->EndArray();
  writer->EndObject();
}

bool UserGroupDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, shape.USER_NAME, m_userName)) {
    LOG_ERROR("failed to deserialize user_name");
    success = false;
  }
  if (const auto *groupAdds = json::member(obj, shape.GROUPS_ADD); groupAdds != nullptr && groupAdds->IsArray()) {
    for (const auto &item : groupAdds->GetArray()) {
      if (item.IsString()) { m_groupAdds.insert(std::string(item.GetString(), item.GetStringLength())); }
    }
  } else {
    LOG_ERROR("failed to deserialize user_name");
    success = false;
  }
  if (const auto *groupRemoves = json::member(obj, shape.GROUPS_REMOVE);
      groupRemoves != nullptr && groupRemoves->IsArray()) {
    for (const auto &item : groupRemoves->GetArray()) {
      if (item.IsString()) { m_groupRemoves.insert(std::string(item.GetString(), item.GetStringLength())); }
    }
  } else {
    LOG_ERROR("failed to deserialize user_name");
    success = false;
  }
  return success;
}
}// namespace lintel
