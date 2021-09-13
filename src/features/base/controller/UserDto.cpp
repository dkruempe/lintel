#include "base_library/features/base/controller/UserDto.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/StringifyService.h"

UserDto::Shapes UserDto::shape{};
const std::string& UserDto::getFirstName() const { return m_firstName; }
const std::string& UserDto::getLastName() const { return m_lastName; }
const std::string& UserDto::getEMail() const { return m_eMail; }
const std::string& UserDto::getUserName() const { return m_userName; }
const date::sys_time<std::chrono::microseconds>& UserDto::getCreatedTimestamp()
    const {
  return m_createdTimestamp;
}
UserDto::UserDto(const User& user, std::string id)
    : JsonSerializable(),
      m_firstName(user.getFirstName()),
      m_lastName(user.getLastName()),
      m_eMail(user.getEmail()),
      m_userName(user.getUserName()),
      m_id(std::move(id)),
      m_createdTimestamp(user.getCreatedTimestamp()),
      m_groups(user.getGroups().empty()
                   ? nullptr
                   : std::make_shared<GroupsDto>(user.getGroups())) {}
void UserDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartObject();
  // FIRST_NAME
  writer->String(shape.FIRST_NAME.c_str());
  writer->String(m_firstName.c_str());
  // LAST_NAME
  writer->String(shape.LAST_NAME.c_str());
  writer->String(m_lastName.c_str());
  // EMAIL
  writer->String(shape.EMAIL.c_str());
  writer->String(m_eMail.c_str());
  // USER_NAME
  writer->String(shape.USER_NAME.c_str());
  writer->String(m_userName.c_str());
  // ID
  if (!m_id.empty()) {
    writer->String(shape.ID.c_str());
    writer->String(m_id.c_str());
  }
  // CREATED_TIMESTAMP
  writer->String(shape.CREATED_TIMESTAMP.c_str());
  writer->String(StringifyService<date::sys_time<std::chrono::microseconds>>::
                     serializeToString(getCreatedTimestamp())
                         .c_str());
  // GROUPS
  if (m_groups != nullptr) {
    writer->String(shape.GROUPS.c_str());
    m_groups->serialize(writer);
  }
  writer->EndObject();
}
bool UserDto::deserialize(const rapidjson::Value& obj) {
  bool success = true;
  // FIRST_NAME
  if (obj.HasMember(shape.FIRST_NAME.c_str())) {
    m_firstName = obj[shape.FIRST_NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serializatioon", shape.FIRST_NAME);
  }
  // LAST_NAME
  if (obj.HasMember(shape.LAST_NAME.c_str())) {
    m_lastName = obj[shape.LAST_NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serializatioon", shape.LAST_NAME);
  }
  // EMAIL
  if (obj.HasMember(shape.EMAIL.c_str())) {
    m_eMail = obj[shape.EMAIL.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serializatioon", shape.EMAIL);
  }
  // USER_NAME
  if (obj.HasMember(shape.USER_NAME.c_str())) {
    m_userName = obj[shape.USER_NAME.c_str()].GetString();
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serializatioon", shape.USER_NAME);
  }
  // CREATED_TIMESTAMP
  if (obj.HasMember(shape.CREATED_TIMESTAMP.c_str())) {
    m_createdTimestamp =
        StringifyService<date::sys_time<std::chrono::microseconds>>::
            deserializeFromString(
                obj[shape.CREATED_TIMESTAMP.c_str()].GetString());
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serializatioon", shape.CREATED_TIMESTAMP);
  }
  // GROUPS
  if (obj.HasMember(shape.GROUPS.c_str())) {
    m_groups = std::make_unique<GroupsDto>();
    m_groups->deserialize(obj[shape.GROUPS.c_str()]);
  } else {
    m_groups = nullptr;
  }
  // ID
  if (obj.HasMember(shape.ID.c_str())) {
    m_id = obj[shape.ID.c_str()].GetString();
  } else {
    m_id = "";
  }
  return success;
}
const std::string& UserDto::getId() const { return m_id; }
const std::vector<GroupDto>& UserDto::getGroups() const {
  return m_groups->getGroups();
}
std::vector<UserDto> UsersDto::init(const std::vector<User>& users) {
  std::vector<UserDto> userDtos;
  userDtos.reserve(users.size());
  for (auto& user : users) {
    userDtos.emplace_back(user);
  }
  return userDtos;
}
UsersDto::UsersDto(const std::vector<User>& users) : m_users(init(users)) {}
const std::vector<UserDto>& UsersDto::getUsers() const { return m_users; }
void UsersDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartArray();
  for (const auto& item : m_users) {
    item.serialize(writer);
  }
  writer->EndArray();
}
bool UsersDto::deserialize(const rapidjson::Value& obj) {
  if (!obj.IsArray()) {
    return false;
  }
  for (auto iter = obj.Begin(); iter != obj.End(); iter++) {
    UserDto userDto;
    userDto.deserialize(*iter);
    m_users.push_back(std::move(userDto));
  }
  return true;
}