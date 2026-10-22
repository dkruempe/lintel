#include "lintel/features/http/controllers/UserController.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <utility>

#include "lintel/core/utils/Cryption.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/core/utils/StringUtils.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/base/controller/UserGroupDto.h"
#include "lintel/features/base/controller/UserLoginDto.h"
#include "lintel/features/base/controller/UserNameDto.h"
#include "lintel/features/base/controller/UserPasswordChangeDto.h"
#include "lintel/features/base/controller/UserSessionDto.h"
#include "lintel/features/base/controller/UserSessionsDto.h"
#include "lintel/features/base/controller/UserTokenDto.h"
#include "lintel/features/http/service/HttpStatusCodes.h"

namespace lintel {
namespace {
  /**
   * @return {"group_name":"<group>","removed":<removed>}
   *
   * rapidjson writes the body so that a group name coming from the request
   * (quotes, backslashes, control characters) cannot break the JSON.
   */
  std::string userGroupRemovedBody(const std::string &groupName, bool removed)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("group_name");
    writer.String(groupName.c_str(), static_cast<rapidjson::SizeType>(groupName.size()));
    writer.Key("removed");
    writer.Bool(removed);
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }

  /** @return {"group_name":"<group>","sub_group_name":"<sub>","<key>":<value>} */
  std::string
    userSubGroupBody(const std::string &groupName, const std::string &subGroupName, const char *key, bool value)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("group_name");
    writer.String(groupName.c_str(), static_cast<rapidjson::SizeType>(groupName.size()));
    writer.Key("sub_group_name");
    writer.String(subGroupName.c_str(), static_cast<rapidjson::SizeType>(subGroupName.size()));
    writer.Key(key);
    writer.Bool(value);
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }

  /** @return true if the group already lists @p subGroupName as a member */
  bool hasSubGroupOf(const Group &group, const std::string &subGroupName)
  {
    const std::vector<Group> &members = group.getGroups();
    return std::any_of(members.begin(), members.end(), [&subGroupName](const Group &member) -> bool {
      return member.getGroupName() == subGroupName;
    });
  }
}// namespace

void UserController::loginOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (user.has_value()) {
    LOG_ERROR("{}-{}: login with logged in user", clientIpOf(request), user->m_id);
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  try {
    std::string auth = request.get_header_value("Authorization");
    // remove 'Basic ' prefix => start pos 6
    auth = auth.substr(6);
    std::string result = Cryption::decodeBase64(auth);
    auto found = result.find(':', 0);
    if (found == std::string::npos) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    std::string userName = result.substr(0, found);
    std::string password = result.substr(found + 1);
    UserLogin userLogin{ clientIpOf(request), userName, password };
    LOG_INFO("login {} from {}", userName, clientIpOf(request));
    std::optional<UserToken> userToken = m_authService->onLoginOf(userLogin);
    if (!userToken.has_value()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    LOG_INFO("{}-{}: successfull login", userToken.value().m_ipAddress, userToken.value().m_id);
    UserDto userDto(userToken.value().m_user, userToken.value().m_id);
    const std::string responseBody = userDto.JsonSerializable::serialize();
    response.set_content(responseBody, contentType.getName());
  } catch (const std::exception &exception) {
    LOG_ERROR("login failed: {}", exception.what());
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

UserController::UserController(const std::shared_ptr<IAuthService> &authService,
  std::shared_ptr<GroupRepository> groupRepository,
  std::shared_ptr<UserRepository> userRepository)
  : Controller(authService), m_groupRepository(std::move(groupRepository)), m_userRepository(std::move(userRepository)),
    m_adminUser("Admin-User", {}, true), m_userUser("User-User", {}, true)
{
  add(m_adminUser);
  add(m_userUser);
}

void UserController::logoutOfDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  UserTokenLogin userTokenLogin{ clientIpOf(request), user->m_id };
  m_authService->onLogoutOf(userTokenLogin);
}

void UserController::allGroupsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    GroupsDto groupsDto(m_groupRepository->allOf());
    response.set_content(groupsDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::allGroupsOfGroupNameOrIsVirtualGroupGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    std::string temp = request.matches[1];
    GroupsDto groupsDto;
    if (temp == "true" || temp == "false") {
      groupsDto = GroupsDto(m_groupRepository->allOf(temp == "true"));
    } else {
      groupsDto = GroupsDto(m_groupRepository->allOf(temp));
    }
    response.set_content(groupsDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::allGroupsOfGroupNameAndIsVirtualGroupGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    std::string groupName = request.matches[1];
    bool isVirtual = request.matches[2] == "true";
    GroupsDto groupsDto(m_groupRepository->allOf(groupName, isVirtual));
    response.set_content(groupsDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::createGroupOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  rapidjson::Document document;
  document.Parse(request.body.c_str());
  if (document.HasParseError() || !document.IsObject()) {
    LOG_WARN("{} sent an unparsable group body", user->m_user.getUserName());
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  std::string groupName;
  if (!json::getString(document, "group_name", groupName) || groupName.empty()) {
    LOG_WARN("{} sent a group without a name", user->m_user.getUserName());
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  // The virtual flag is optional and defaults to a regular (persisted) group.
  bool isVirtual = false;
  json::getBool(document, "virtual", isVirtual);
  if (isVirtual) {
    // Virtual groups carry access rights and are defined in the code, so the
    // repository rejects creating them over the API.
    LOG_WARN("{} tried to create the virtual group {}", user->m_user.getUserName(), groupName);
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  if (m_groupRepository->of(groupName).has_value()) {
    LOG_WARN("{} tried to create the existing group {}", user->m_user.getUserName(), groupName);
    response.status = HttpStatusCodes::Conflict;
    response.set_content("", contentType.getName());
    return;
  }
  const Group group(groupName, {}, false);
  m_groupRepository->createOf(group);
  GroupDto groupDto(group);
  response.status = HttpStatusCodes::OK;
  response.set_content(groupDto.JsonSerializable::serialize(), contentType.getName());
}

void UserController::deleteGroupOfDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string groupName = request.matches[1];
  const std::optional<Group> group = m_groupRepository->of(groupName);
  if (!group.has_value()) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  try {
    m_groupRepository->deleteOf(group.value());
  } catch (const std::exception &exception) {
    // deleteOf() refuses groups that are still assigned to a user
    LOG_WARN("{} failed to delete group {}: {}", user->m_user.getUserName(), groupName, exception.what());
    response.status = HttpStatusCodes::Conflict;
    response.set_content("", contentType.getName());
    return;
  }
  LOG_INFO("{} deleted group {}", user->m_user.getUserName(), groupName);
  response.status = HttpStatusCodes::OK;
  response.set_content(userGroupRemovedBody(groupName, true), contentType.getName());
}

void UserController::addGroupMemberOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string groupName = request.matches[1];
  const std::string subGroupName = request.matches[2];
  const std::optional<Group> group = m_groupRepository->of(groupName);
  if (!group.has_value()) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  const std::optional<Group> subGroup = m_groupRepository->of(subGroupName);
  if (!subGroup.has_value()) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  if (group->isVirtual()) {
    // Virtual groups are the leaves of the hierarchy, they cannot collect members
    LOG_WARN("{} tried to add a member to the virtual group {}", user->m_user.getUserName(), groupName);
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  if (!subGroup->isVirtual()) {
    // Only virtual groups can be a member: they carry the access rights
    LOG_WARN("{} tried to add the non virtual group {} to {}", user->m_user.getUserName(), subGroupName, groupName);
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  const bool alreadyMember = hasSubGroupOf(group.value(), subGroupName);
  if (!alreadyMember) { m_groupRepository->addGroupOf(group.value(), subGroup.value()); }
  response.status = HttpStatusCodes::OK;
  response.set_content(userSubGroupBody(groupName, subGroupName, "added", !alreadyMember), contentType.getName());
}

void UserController::removeGroupMemberOfDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string groupName = request.matches[1];
  const std::string subGroupName = request.matches[2];
  const std::optional<Group> group = m_groupRepository->of(groupName);
  if (!group.has_value()) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  const bool wasMember = hasSubGroupOf(group.value(), subGroupName);
  if (wasMember) {
    const std::optional<Group> subGroup = m_groupRepository->of(subGroupName);
    if (!subGroup.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      return;
    }
    m_groupRepository->removeGroupOf(group.value(), subGroup.value());
  }
  response.status = HttpStatusCodes::OK;
  response.set_content(userSubGroupBody(groupName, subGroupName, "removed", wasMember), contentType.getName());
}

void UserController::allUsersOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    PagingParams paging = pagingParamsOf(request);
    if (paging.m_enabled) {
      Page<User> page = m_userRepository->pageOf("", paging.m_after, paging.m_limit);
      UsersPageDto usersPageDto(page);
      response.set_content(usersPageDto.JsonSerializable::serialize(), contentType.getName());
      return;
    }
    UsersDto usersDto(m_userRepository->allOf());
    response.set_content(usersDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::allUsersOfUserNameGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    std::string userName = request.matches[1];
    PagingParams paging = pagingParamsOf(request);
    if (paging.m_enabled) {
      Page<User> page = m_userRepository->pageOf(userName, paging.m_after, paging.m_limit);
      UsersPageDto usersPageDto(page);
      response.set_content(usersPageDto.JsonSerializable::serialize(), contentType.getName());
      return;
    }
    UsersDto usersDto(m_userRepository->allOf(userName));
    response.set_content(usersDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::deleteUserDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  LOG_TRACE("received user delete");
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    UserNamesDto userNamesDto;
    userNamesDto.JsonSerializable::deserialize(request.body);
    std::vector<UserNameDto> userNames = userNamesDto.getUserNames();
    std::vector<std::string> userNamesString;
    userNamesString.reserve(userNames.size());
    for (const auto &userName : userNames) {
      userNamesString.push_back(userName.getUserName());
      LOG_TRACE("user_name={}", userName.getUserName());
    }
    auto found = std::find_if(userNamesString.begin(),
      userNamesString.end(),
      [&user](const std::string &userName) -> bool { return user.value().m_user.getUserName() == userName; });
    if (found != userNamesString.end()) {
      LOG_WARN("{} abort delete user bc. current user wanted to delete himself", user->m_user.getUserName());
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    m_userRepository->deleteOf(userNamesString);
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::addUserPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    UserDto userDto;
    userDto.JsonSerializable::deserialize(request.body);
    // check if password is set
    auto userPassword = userDto.getPassword();
    if (!userPassword.has_value()) {
      response.status = HttpStatusCodes::NotAcceptable;
      response.set_content("", contentType.getName());
      return;
    }
    std::string password = Cryption::hashOf(Cryption::decodeBase64(userPassword.value()));
    User userNew(userDto.getFirstName(),
      userDto.getLastName(),
      userDto.getSex(),
      userDto.getEMail(),
      userDto.getUserName(),
      password,
      {});
    std::stringstream ss;
    ss << userNew;
    LOG_TRACE("{}", ss.str());
    m_userRepository->createOf(userNew);
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::updateUserPut(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    UserGroupDto userGroupDto;
    userGroupDto.JsonSerializable::deserialize(request.body);
    // check user
    std::optional<User> userTemp = m_userRepository->of(userGroupDto.getUserName());
    if (!userTemp.has_value()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    // transform to group
    std::set<std::string> groupAddStrings = userGroupDto.getGroupAdds();
    std::set<std::string> groupRemoveStrings = userGroupDto.getGroupRemoves();
    std::set<Group> groupAdds;
    for (auto &iter : groupAddStrings) {
      auto optGroup = m_groupRepository->of(iter);
      if (!optGroup.has_value()) { continue; }
      groupAdds.insert(optGroup.value());
    }
    std::set<Group> groupRemoves;
    for (auto &iter : groupRemoveStrings) {
      auto optGroup = m_groupRepository->of(iter);
      if (!optGroup.has_value()) { continue; }
      groupRemoves.insert(optGroup.value());
    }
    // operate
    m_userRepository->addGroupsOf(userTemp.value(), groupAdds);
    m_userRepository->removeGroupsOf(userTemp.value(), groupRemoves);
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::changePasswordOfPut(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    UserPasswordChangeDto passwordChangeDto;
    passwordChangeDto.JsonSerializable::deserialize(request.body);
    const std::string &userName = passwordChangeDto.getUserName();
    if (userName.empty()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    std::optional<User> userTemp = m_userRepository->of(userName);
    if (!userTemp.has_value()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    const bool isAdmin = user->m_user.has(m_adminUser);
    const bool isSelfService = userName == user->m_user.getUserName();
    if (!isAdmin && !isSelfService) {
      LOG_WARN("{} tried to change password of {}", user->m_user.getUserName(), userName);
      response.status = HttpStatusCodes::Unauthorized;
      response.set_content("", contentType.getName());
      return;
    }
    const std::string newPassword = Cryption::decodeBase64(passwordChangeDto.getNewPassword());
    if (newPassword.empty()) {
      response.status = HttpStatusCodes::NotAcceptable;
      response.set_content("", contentType.getName());
      return;
    }
    if (isSelfService
        && !Cryption::verifyOf(Cryption::decodeBase64(passwordChangeDto.getOldPassword()), userTemp->getPassword())) {
      LOG_WARN("{} failed to change password of {} bc. of wrong old password", user->m_user.getUserName(), userName);
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    m_userRepository->changePasswordOf(userTemp.value(), Cryption::hashOf(newPassword));
    response.status = HttpStatusCodes::OK;
    response.set_content("", contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::allSessionsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType == ContentType::ApplicationJson) {
    UserSessionsDto sessionsDto(UserSessionDto::listOf(m_authService->allTokensOf(user->m_user.getUserName())));
    response.set_content(sessionsDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void UserController::revokeSessionOfDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string id = request.matches[1];
  const std::vector<UserToken> ownTokens = m_authService->allTokensOf(user->m_user.getUserName());
  const bool isOwnSession =
    std::any_of(ownTokens.begin(), ownTokens.end(), [&id](const UserToken &token) { return token.m_id == id; });
  if (!isOwnSession && !user->m_user.has(m_adminUser)) {
    LOG_WARN("{} tried to revoke foreign session {}", user->m_user.getUserName(), id);
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  m_authService->revokeTokenOf(id);
  response.status = HttpStatusCodes::OK;
  response.set_content("", contentType.getName());
}

void UserController::loginStateOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  response.status = HttpStatusCodes::OK;
  response.set_content("", contentType.getName());
}
}// namespace lintel
