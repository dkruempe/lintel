#include "base_library/features/http/controllers/UserController.h"

#include <algorithm>
#include <utility>

#include "base_library/core/utils/Cryption.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserGroupDto.h"
#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/base/controller/UserNameDto.h"
#include "base_library/features/base/controller/UserPasswordChangeDto.h"
#include "base_library/features/base/controller/UserSessionDto.h"
#include "base_library/features/base/controller/UserSessionsDto.h"
#include "base_library/features/base/controller/UserTokenDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"

void UserController::loginOfPost(const httplib::Request &request,
                                 httplib::Response &response,
                                 const ContentType &contentType,
                                 const std::optional<UserToken> &user) {
    if (user.has_value()) {
        LOG_ERROR("{}-{}: login with logged in user", clientIpOf(request),
                  user->m_id);
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
        UserLogin userLogin{clientIpOf(request), userName, password};
        LOG_INFO("login {} from {}", userName, clientIpOf(request));
        std::optional<UserToken> userToken = m_authService->onLoginOf(userLogin);
        if (!userToken.has_value()) {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            return;
        }
        LOG_INFO("{}-{}: successfull login", userToken.value().m_ipAddress,
                 userToken.value().m_id);
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
        : Controller(authService),
          m_groupRepository(std::move(groupRepository)),
          m_userRepository(std::move(userRepository)),
          m_adminUser("Admin-User", {}, true),
          m_userUser("User-User", {}, true) {
    add(m_adminUser);
    add(m_userUser);
}

void UserController::logoutOfDelete(const httplib::Request &request,
                                    httplib::Response &response,
                                    const ContentType &contentType,
                                    const std::optional<UserToken> &user) {
    if (!user.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    UserTokenLogin userTokenLogin{clientIpOf(request), user->m_id};
    m_authService->onLogoutOf(userTokenLogin);
}

void UserController::allGroupsOfGet(const httplib::Request &request,
                                    httplib::Response &response,
                                    const ContentType &contentType,
                                    const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            GroupsDto groupsDto(m_groupRepository->allOf());
            response.set_content(groupsDto.JsonSerializable::serialize(),
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::allGroupsOfGroupNameOrIsVirtualGroupGet(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &user) {
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
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::allGroupsOfGroupNameAndIsVirtualGroupGet(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            std::string groupName = request.matches[1];
            bool isVirtual = request.matches[2] == "true";
            GroupsDto groupsDto(m_groupRepository->allOf(groupName, isVirtual));
            response.set_content(groupsDto.JsonSerializable::serialize(),
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::allUsersOfGet(const httplib::Request &request,
                                   httplib::Response &response,
                                   const ContentType &contentType,
                                   const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            UsersDto usersDto(m_userRepository->allOf());
            response.set_content(usersDto.JsonSerializable::serialize(),
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::allUsersOfUserNameGet(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            std::string userName = request.matches[1];
            UsersDto usersDto(m_userRepository->allOf(userName));
            response.set_content(usersDto.JsonSerializable::serialize(),
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::deleteUserDelete(const httplib::Request &request,
                                      httplib::Response &response,
                                      const ContentType &contentType,
                                      const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            UserNamesDto userNamesDto;
            userNamesDto.JsonSerializable::deserialize(request.body);
            std::vector<UserNameDto> userNames = userNamesDto.getUserNames();
            std::vector<std::string> userNamesString;
            userNamesString.reserve(userNames.size());
            for (const auto &userName: userNames) {
                userNamesString.push_back(userName.getUserName());
                LOG_TRACE("user_name={}", userName.getUserName());
            }
            auto found =
                    std::find_if(userNamesString.begin(), userNamesString.end(),
                                 [&user](const std::string &userName) -> bool {
                                     return user.value().m_user.getUserName() == userName;
                                 });
            if (found != userNamesString.end()) {
                LOG_WARN(
                        "{} abort delete user bc. current user wanted to delete himself",
                        user->m_user.getUserName());
                response.status = HttpStatusCodes::Forbidden;
                response.set_content("", contentType.getName());
                break;
            }
            m_userRepository->deleteOf(userNamesString);
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::addUserPost(const httplib::Request &request,
                                 httplib::Response &response,
                                 const ContentType &contentType,
                                 const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            UserDto userDto;
            userDto.JsonSerializable::deserialize(request.body);
            // check if password is set
            auto userPassword = userDto.getPassword();
            if (!userPassword.has_value()) {
                response.status = HttpStatusCodes::NotAcceptable;
                response.set_content("", contentType.getName());
                return;
            }
            std::string password = Cryption::hashOf(
                    Cryption::decodeBase64(userPassword.value()));
            User userNew(userDto.getFirstName(), userDto.getLastName(),
                         userDto.getSex(), userDto.getEMail(), userDto.getUserName(),
                         password, {});
            std::stringstream ss;
            ss << userNew;
            LOG_TRACE("{}", ss.str());
            m_userRepository->createOf(userNew);
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::updateUserPut(const httplib::Request &request,
                                   httplib::Response &response,
                                   const ContentType &contentType,
                                   const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            UserGroupDto userGroupDto;
            userGroupDto.JsonSerializable::deserialize(request.body);
            // check user
            std::optional<User> userTemp =
                    m_userRepository->of(userGroupDto.getUserName());
            if (!userTemp.has_value()) {
                response.status = HttpStatusCodes::Forbidden;
                response.set_content("", contentType.getName());
                return;
            }
            // transform to group
            std::set<std::string> groupAddStrings = userGroupDto.getGroupAdds();
            std::set<std::string> groupRemoveStrings = userGroupDto.getGroupRemoves();
            std::set<Group> groupAdds;
            for (auto &iter: groupAddStrings) {
                auto optGroup = m_groupRepository->of(iter);
                if (!optGroup.has_value()) {
                    continue;
                }
                groupAdds.insert(optGroup.value());
            }
            std::set<Group> groupRemoves;
            for (auto &iter: groupRemoveStrings) {
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
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::changePasswordOfPut(const httplib::Request &request,
                                         httplib::Response &response,
                                         const ContentType &contentType,
                                         const std::optional<UserToken> &user) {
    if (!user.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    switch (contentType) {
        case ContentType::ApplicationJson: {
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
                LOG_WARN("{} tried to change password of {}", user->m_user.getUserName(),
                         userName);
                response.status = HttpStatusCodes::Unauthorized;
                response.set_content("", contentType.getName());
                return;
            }
            const std::string newPassword =
                    Cryption::decodeBase64(passwordChangeDto.getNewPassword());
            if (newPassword.empty()) {
                response.status = HttpStatusCodes::NotAcceptable;
                response.set_content("", contentType.getName());
                return;
            }
            if (isSelfService &&
                !Cryption::verifyOf(Cryption::decodeBase64(passwordChangeDto.getOldPassword()),
                                    userTemp->getPassword())) {
                LOG_WARN("{} failed to change password of {} bc. of wrong old password",
                         user->m_user.getUserName(), userName);
                response.status = HttpStatusCodes::Forbidden;
                response.set_content("", contentType.getName());
                return;
            }
            m_userRepository->changePasswordOf(userTemp.value(),
                                               Cryption::hashOf(newPassword));
            response.status = HttpStatusCodes::OK;
            response.set_content("", contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::allSessionsOfGet(const httplib::Request &request,
                                      httplib::Response &response,
                                      const ContentType &contentType,
                                      const std::optional<UserToken> &user) {
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
    switch (contentType) {
        case ContentType::ApplicationJson: {
            UserSessionsDto sessionsDto(
                    UserSessionDto::listOf(m_authService->allTokensOf(
                            user->m_user.getUserName())));
            response.set_content(sessionsDto.JsonSerializable::serialize(),
                                 contentType.getName());
            break;
        }
        default: {
            response.status = HttpStatusCodes::Forbidden;
            response.set_content("", contentType.getName());
            break;
        }
    }
}

void UserController::revokeSessionOfDelete(const httplib::Request &request,
                                           httplib::Response &response,
                                           const ContentType &contentType,
                                           const std::optional<UserToken> &user) {
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
    const std::vector<UserToken> ownTokens =
            m_authService->allTokensOf(user->m_user.getUserName());
    const bool isOwnSession =
            std::any_of(ownTokens.begin(), ownTokens.end(),
                        [&id](const UserToken &token) {
                            return token.m_id == id;
                        });
    if (!isOwnSession && !user->m_user.has(m_adminUser)) {
        LOG_WARN("{} tried to revoke foreign session {}",
                 user->m_user.getUserName(), id);
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
                                     const std::optional<UserToken> &user) {
    if (!user.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    response.status = HttpStatusCodes::OK;
    response.set_content("", contentType.getName());
}
