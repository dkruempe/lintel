#include "base_library/features/base/controller/UserNameDto.h"

#include "base_library/features/base/controller/UserDto.h"

UserNameDto::Shapes UserNameDto::shape{};

UserNameDto::UserNameDto(std::string userName)
  : m_userName(std::move(userName)) {}

const std::string &UserNameDto::getUserName() const { return m_userName; }

void UserNameDto::serialize(
  rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // USERNAME
  writer->String(shape.USER_NAME.c_str());
  writer->String(m_userName.c_str());
  writer->EndObject();
}

bool UserNameDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // USERNAME
  if (obj.HasMember(shape.USER_NAME.c_str())) { m_userName = obj[shape.USER_NAME.c_str()].GetString(); } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.USER_NAME.c_str());
  }
  return success;
}

UserNameDto::UserNameDto(const User &user)
  : m_userName(user.getUserName()) {}

UserNamesDto::UserNamesDto(const std::vector<std::string> &userNames)
  : m_userNames(init(userNames)) {}

std::vector<UserNameDto> UserNamesDto::init(const std::vector<User> &users)
{
  std::vector<UserNameDto> userNames;
  userNames.reserve(users.size());
  std::transform(users.begin(),
    users.end(),
    std::back_inserter(userNames),
    [](const User &user) { return UserNameDto(user); });
  return userNames;
}

std::vector<UserNameDto> UserNamesDto::init(
  const std::vector<std::string> &users)
{
  std::vector<UserNameDto> userNames;
  userNames.reserve(users.size());
  std::transform(users.begin(),
    users.end(),
    userNames.begin(),
    [](const std::string &user) -> UserNameDto { return UserNameDto(user); });
  return userNames;
}

UserNamesDto::UserNamesDto(const std::vector<User> &users)
  : m_userNames(init(users)) {}

void UserNamesDto::serialize(
  rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &user : m_userNames) { user.serialize(writer); }
  writer->EndArray();
}

bool UserNamesDto::deserialize(const rapidjson::Value &obj)
{
  if (!obj.IsArray()) { return false; }
  for (auto iter = obj.Begin(); iter != obj.End(); iter++) {
    UserNameDto userNameDto;
    userNameDto.deserialize(*iter);
    m_userNames.push_back(std::move(userNameDto));
  }
  return true;
}

const std::vector<UserNameDto> &UserNamesDto::getUserNames() const { return m_userNames; }