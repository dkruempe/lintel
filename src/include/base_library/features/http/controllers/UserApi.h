#ifndef CPP_BASE_LIBRARY_USERAPI_H
#define CPP_BASE_LIBRARY_USERAPI_H

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/base/controller/UserPasswordChangeDto.h"
#include "base_library/features/base/controller/UserSessionDto.h"
#include "base_library/features/base/controller/UserTokenDto.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/http/provider/ClientProvider.h"

/** API client for user authentication and management via HTTP */
class UserApi {
private:
    std::shared_ptr<Client> m_client;

protected:
    UserApi() = default;

public:
    explicit UserApi(const std::shared_ptr<ClientProvider> &clientProvider);

    // basic user functions for login / logout
    /** @param userLoginDto login credentials; return authenticated user if successful */
    virtual std::optional<UserDto> loginOf(const UserLoginDto &userLoginDto);

    /** @param userTokenDto token to invalidate; return true on success */
    virtual bool logoutOf(const UserTokenDto &userTokenDto);

    /**
     * Change a user's password.
     * @param passwordChangeDto target user, old (self-service) and new password
     * @return true on success
     */
    virtual bool changePasswordOf(const UserPasswordChangeDto &passwordChangeDto);

    /** @return all active sessions of the current user */
    virtual std::vector<UserSessionDto> sessionsOf();

    /** @param sessionId id of the session to revoke; return true on success */
    virtual bool revokeSessionOf(const std::string &sessionId);

    /** @return true if a user is currently logged in */
    virtual bool isLoggedIn();

    // user management functions
    /** @return all groups */
    virtual std::vector<GroupDto> allOf();

    /** @param groupName filter by name; return matching groups */
    virtual std::vector<GroupDto> allOf(const std::string &groupName);

    /** @param groupName filter by name; @param isVirtualGroup filter by virtual flag */
    virtual std::vector<GroupDto> allOf(const std::string &groupName,
                                bool isVirtualGroup);

    /** @param isVirtualGroup filter by virtual flag; return matching groups */
    virtual std::vector<GroupDto> allOf(bool isVirtualGroup);

    /** @return all users */
    virtual std::vector<UserDto> allUsersOf();

    /** @param userName filter by name; return matching users */
    virtual std::vector<UserDto> allUsersOf(const std::string &userName);

    /** Create a new user */
    virtual void createOf(const UserDto &userDto);

    /**
     * Update user group memberships
     * @param userName user to update
     * @param addGroups groups to add
     * @param removeGroups groups to remove
     */
    virtual void updateOf(const std::string &userName,
                  const std::set<std::string> &addGroups,
                  const std::set<std::string> &removeGroups);

    /** Delete users by name */
    virtual void deleteOf(const std::vector<std::string> &userNames);
};

#endif  // CPP_BASE_LIBRARY_USERAPI_H
