#include "base_library/features/base/controller/UserController.h"

#include <algorithm>
#include <utility>

#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/base/configuration/Cryption.h"
#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserGroupDto.h"
#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/base/controller/UserTokenDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"

void UserController::loginOfPost(const httplib::Request& request,
                                 httplib::Response& response,
                                 const ContentType& contentType,
                                 const std::optional<UserToken>& user) {
  if (user.has_value()) {
    LOG_ERROR("{}-{}: login with logged in user", request.remote_addr,
              user->m_id);
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName().c_str());
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
      response.set_content("", contentType.getName().c_str());
      return;
    }
    std::string userName = result.substr(0, found);
    std::string password = result.substr(found + 1);
    UserLogin userLogin{request.remote_addr, userName, password};
    LOG_INFO("login {} {} {}", userName, password, request.remote_addr);
    std::optional<UserToken> userToken = m_authService->onLoginOf(userLogin);
    if (!userToken.has_value()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      return;
    }
    LOG_INFO("{}-{}: successfull login", userToken.value().m_ipAddress,
             userToken.value().m_id);
    UserDto userDto(userToken.value().m_user, userToken.value().m_id);
    const std::string responseBody = userDto.JsonSerializable::serialize();
    response.set_content(responseBody, "application/json");
  } catch (const std::exception& exception) {
    LOG_ERROR("login failed {} for {}", exception.what(), request.body);
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName().c_str());
  }
}
UserController::UserController(const std::shared_ptr<AuthService>& authService,
                               std::shared_ptr<GroupRepository> groupRepository,
                               std::shared_ptr<UserRepository> userRepository)
    : Controller(authService),
      m_authService(authService),
      m_groupRepository(std::move(groupRepository)),
      m_userRepository(std::move(userRepository)),
      m_adminUser("Admin-User", {}, true),
      m_userUser("User-User", {}, true) {
  add(m_adminUser);
  add(m_userUser);
}
void UserController::logoutOfDelete(const httplib::Request& request,
                                    httplib::Response& response,
                                    const ContentType& contentType,
                                    const std::optional<UserToken>& user) {
  UserTokenLogin userTokenLogin{request.remote_addr, user->m_id};
  m_authService->onLogoutOf(userTokenLogin);
}
void UserController::allGroupsOfGet(const httplib::Request& request,
                                    httplib::Response& response,
                                    const ContentType& contentType,
                                    const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      GroupsDto groupsDto(m_groupRepository->allOf());
      response.set_content(groupsDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::allGroupsOfGroupNameOrIsVirtualGroupGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      std::string temp = request.matches[1];
      GroupsDto groupsDto;
      if (temp == "true" || temp == "false") {
        groupsDto = GroupsDto(m_groupRepository->allOf(temp == "true"));
      } else {
        groupsDto = GroupsDto(m_groupRepository->allOf(temp));
      }
      response.set_content(groupsDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::allGroupsOfGroupNameAndIsVirtualGroupGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      std::string groupName = request.matches[1];
      bool isVirtual = request.matches[2] == "true";
      GroupsDto groupsDto(m_groupRepository->allOf(groupName, isVirtual));
      response.set_content(groupsDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::allUsersOfGet(const httplib::Request& request,
                                   httplib::Response& response,
                                   const ContentType& contentType,
                                   const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      UsersDto usersDto(m_userRepository->allOf());
      response.set_content(usersDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::allUsersOfUserNameGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      std::string userName = request.matches[1];
      UsersDto usersDto(m_userRepository->allOf(userName));
      response.set_content(usersDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::addUserPost(const httplib::Request& request,
                                 httplib::Response& response,
                                 const ContentType& contentType,
                                 const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      UserDto userDto;
      userDto.JsonSerializable::deserialize(request.body);
      // check if password is set
      if (!userDto.getPassword().has_value()) {
        response.status = HttpStatusCodes::NotAcceptable;
        response.set_content("", contentType.getName().c_str());
        return;
      }
      std::string password = Cryption::hashOf(
          Cryption::decodeBase64(userDto.getPassword().value()));
      User userNew(userDto.getFirstName(), userDto.getLastName(),
                   User::Sex::Male, userDto.getEMail(), userDto.getUserName(),
                   password, {});
      m_userRepository->createOf(userNew);
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::updateUserPut(const httplib::Request& request,
                                   httplib::Response& response,
                                   const ContentType& contentType,
                                   const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  switch (contentType) {
    case ContentType::ApplicationJson: {
      UserGroupDto userGroupDto;
      userGroupDto.JsonSerializable::deserialize(request.body);
      // check user
      std::optional<User> userTemp =
          m_userRepository->of(userGroupDto.getUserName());
      if (!userTemp.has_value()) {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName().c_str());
        return;
      }
      // transform to group
      std::set<std::string> groupAddStrings = userGroupDto.getGroupAdds();
      std::set<std::string> groupRemoveStrings = userGroupDto.getGroupRemoves();
      std::set<Group> groupAdds;
      for (auto& iter : groupAddStrings) {
        auto optGroup = m_groupRepository->of(iter);
        if (!optGroup.has_value()) {
          continue;
        }
        groupAdds.insert(optGroup.value());
      }
      std::set<Group> groupRemoves;
      for (auto& iter : groupRemoveStrings) {
        auto optGroup = m_groupRepository->of(iter);
        if (!optGroup.has_value()) {
          continue;
        }
        groupRemoves.insert(optGroup.value());
      }
      // operate
      m_userRepository->addGroupsOf(userTemp.value(), groupAdds);
      m_userRepository->removeGroupsOf(userTemp.value(), groupRemoves);
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void UserController::loginStateOfGet(const httplib::Request& request,
                                     httplib::Response& response,
                                     const ContentType& contentType,
                                     const std::optional<UserToken>& user) {
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_adminUser) && !user->m_user.has(m_userUser)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  response.status = HttpStatusCodes::OK;
  response.set_content("", contentType.getName().c_str());
}
