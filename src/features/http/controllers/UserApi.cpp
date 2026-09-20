#include <httplib.h>

#include "base_library/features/http/controllers/UserApi.h"

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserGroupDto.h"
#include "base_library/features/base/controller/UserNameDto.h"
#include "base_library/features/base/controller/UserSessionsDto.h"
#include "base_library/features/http/service/HttpClientHelper.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/http/service/HttpUnauthorizedException.h"

UserApi::UserApi(const std::shared_ptr<ClientProvider> &clientProvider)
        : m_client(clientProvider->provide()) {}

std::optional<UserDto> UserApi::loginOf(const UserLoginDto &userLoginDto) {
    m_client->setBasicAuth(userLoginDto.getUserName(),
                           userLoginDto.getPassword());
    httplib::Result result =
            m_client->post("/user/login", "", "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return std::nullopt;
    }
    if (result->status != HttpStatusCodes::OK) {
        return std::nullopt;
    }
    UserDto userDto;
    userDto.JsonSerializable::deserialize(result.value().body);
    m_client->setBasicAuth("", "");
    m_client->setBearerTokenAuth(userDto.getId());
    return std::make_optional<UserDto>(std::move(userDto));
}

bool UserApi::logoutOf(const UserTokenDto &userTokenDto) {
    httplib::Result result =
            m_client->deletes("/user/logout", "", "application/json");
    return HttpClientHelper::hasResponse(result) && result->status == HttpStatusCodes::OK;
}

bool UserApi::changePasswordOf(
        const UserPasswordChangeDto &passwordChangeDto) {
    std::string body;
    try {
        body = passwordChangeDto.JsonSerializable::serialize();
    } catch (const std::exception &exception) {
        LOG_ERROR("failed to change password of {}",
                  passwordChangeDto.getUserName());
        return false;
    }
    httplib::Result result =
            m_client->put("/user/password", body, "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return false;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            return true;
        default:
            LOG_ERROR("failed to change password of {} ({})",
                      passwordChangeDto.getUserName(), status.getCode());
            return false;
    }
}

std::vector<UserSessionDto> UserApi::sessionsOf() {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/sessions", headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show sessions {}", status.getCode());
            return {};
    }
    UserSessionsDto sessionsDto;
    try {
        sessionsDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return sessionsDto.getSessions();
}

bool UserApi::revokeSessionOf(const std::string &sessionId) {
    httplib::Result result =
            m_client->deletes("/user/sessions/" + sessionId, "", "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return false;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            return true;
        default:
            LOG_ERROR("failed to revoke session {} ({})", sessionId,
                      status.getCode());
            return false;
    }
}

std::vector<GroupDto> UserApi::allOf() {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/groups", headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show groups {}", status.getCode());
            break;
    }
    GroupsDto groupsDto;
    try {
        groupsDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return groupsDto.getGroups();
}

std::vector<UserDto> UserApi::allUsersOf() {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/users", headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show users {}", status.getCode());
            break;
    }
    UsersDto usersDto;
    try {
        usersDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return usersDto.getUsers();
}

std::vector<UserDto> UserApi::allUsersOf(const std::string &userName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/users/" + userName, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show users {}", status.getCode());
            break;
    }
    UsersDto usersDto;
    try {
        usersDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return usersDto.getUsers();
}

std::vector<GroupDto> UserApi::allOf(const std::string &groupName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/groups/" + groupName, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show groups {}", status.getCode());
            break;
    }
    GroupsDto groupsDto;
    try {
        groupsDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return groupsDto.getGroups();
}

std::vector<GroupDto> UserApi::allOf(const std::string &groupName,
                                     bool isVirtualGroup) {
    std::string boolStr = isVirtualGroup ? "true" : "false";
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result =
            m_client->get("/user/groups/" + groupName + "/" + boolStr, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show groups {}", status.getCode());
            break;
    }
    GroupsDto groupsDto;
    try {
        groupsDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return groupsDto.getGroups();
}

std::vector<GroupDto> UserApi::allOf(bool isVirtualGroup) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    std::string boolStr = isVirtualGroup ? "true" : "false";
    httplib::Result result = m_client->get("/user/groups/" + boolStr, headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to show groups {}", status.getCode());
            break;
    }
    GroupsDto groupsDto;
    try {
        groupsDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return groupsDto.getGroups();
}

void UserApi::createOf(const UserDto &userDto) {
    std::string body;
    try {
        body = userDto.JsonSerializable::serialize();
    } catch (std::exception &exception) {
        LOG_ERROR("failed to create user {}", userDto.getUserName());
        return;
    }
    httplib::Result result =
            m_client->post("/user/add", body, "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("{} failed to create user {}", userDto.getUserName(),
                      status.getCode());
            break;
    }
}

void UserApi::updateOf(const std::string &userName,
                       const std::set<std::string> &addGroups,
                       const std::set<std::string> &removeGroups) {
    UserGroupDto userGroupDto(addGroups, removeGroups, userName);
    std::string body = userGroupDto.JsonSerializable::serialize();
    httplib::Result result =
            m_client->put("/user/update", body, "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("{} failed to update user {}", userName, status.getCode());
            break;
    }
}

bool UserApi::isLoggedIn() {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    httplib::Result result = m_client->get("/user/state", headers);
    return HttpClientHelper::hasResponse(result) && result->status == HttpStatusCodes::OK;
}

void UserApi::deleteOf(const std::vector<std::string> &userNames) {
    UserNamesDto userNameDto(userNames);
    std::string body = userNameDto.JsonSerializable::serialize();
    LOG_TRACE("body={}", body);
    httplib::Result result =
            m_client->deletes("/user/delete", body, "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            break;
        default:
            LOG_ERROR("failed to delete users {}", status.getCode());
            break;
    }
}
