#include "base_library/features/base/controller/UserApi.h"

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
UserApi::UserApi(const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
std::optional<UserDto> UserApi::loginOf(const UserLoginDto& userLoginDto) {
  m_client->setBasicAuth(userLoginDto.getUserName(),
                         userLoginDto.getPassword());
  const httplib::Result& result =
      m_client->post("/user/login", "", "application/json");
  if (result->status != HttpStatusCodes::OK) {
    return std::nullopt;
  }
  UserDto userDto;
  userDto.JsonSerializable::deserialize(result.value().body);
  m_client->setBasicAuth("", "");
  m_client->setBearerTokenAuth(userDto.getId());
  return std::make_optional<UserDto>(std::move(userDto));
}
bool UserApi::logoutOf(const UserTokenDto& userTokenDto) {
  httplib::Result result =
      m_client->deletes("/user/logout", "", "application/json");
  return result->status == HttpStatusCodes::OK;
}
std::vector<GroupDto> UserApi::allOf() {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  httplib::Result result = m_client->get("/user/groups", headers);
  GroupsDto groupsDto;
  try {
    groupsDto.JsonSerializable::deserialize(result->body);
  } catch (std::exception& exception) {
    return {};
  }
  return groupsDto.getGroups();
}
std::vector<GroupDto> UserApi::allOf(const std::string& groupName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  httplib::Result result = m_client->get("/user/groups/" + groupName, headers);
  GroupsDto groupsDto;
  try {
    groupsDto.JsonSerializable::deserialize(result->body);
  } catch (std::exception& exception) {
    return {};
  }
  return groupsDto.getGroups();
}
std::vector<GroupDto> UserApi::allOf(const std::string& groupName,
                                     bool isVirtualGroup) {
  std::string boolStr = isVirtualGroup ? "true" : "false";
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  httplib::Result result =
      m_client->get("/user/groups/" + groupName + "/" + boolStr, headers);
  GroupsDto groupsDto;
  try {
    groupsDto.JsonSerializable::deserialize(result->body);
  } catch (std::exception& exception) {
    return {};
  }
  return groupsDto.getGroups();
}
std::vector<GroupDto> UserApi::allOf(bool isVirtualGroup) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  std::string boolStr = isVirtualGroup ? "true" : "false";
  httplib::Result result = m_client->get("/user/groups/" + boolStr, headers);
  GroupsDto groupsDto;
  try {
    groupsDto.JsonSerializable::deserialize(result->body);
  } catch (std::exception& exception) {
    return {};
  }
  return groupsDto.getGroups();
}
