#ifndef CPP_BASE_LIBRARY_IAUTHSERVICE_H
#define CPP_BASE_LIBRARY_IAUTHSERVICE_H

#include <date/date.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base_library/features/base/models/User.h"

/** Credentials for user login. */
struct UserLogin {
    std::string m_ipAddress;  /**< Client IP address */
    std::string m_userName;   /**< Username */
    std::string m_password;   /**< Password */
};

/** Existing token for re-authentication. */
struct UserTokenLogin {
    std::string m_ipAddress;  /**< Client IP address */
    std::string m_id;         /**< Token ID */
};

/** Represents an authenticated user session. */
struct UserToken {
    std::string m_ipAddress;                     /**< Client IP address */
    std::string m_id;                            /**< Token ID */
    date::sys_time<std::chrono::microseconds> m_lastAccessTimestamps; /**< Last access time */
    User m_user;                                 /**< The authenticated user */
};

/**
 * Interface for authentication operations: login, token validation, and logout.
 */
class IAuthService {
public:
    virtual ~IAuthService() = default;

    /**
     * Authenticate a user with credentials.
     * @param userLogin login credentials
     * @return user token if authentication succeeds
     */
    virtual std::optional<UserToken> onLoginOf(const UserLogin &userLogin) = 0;

    /**
     * Validate an access token.
     * @param userTokenLogin token to validate
     * @return user token if valid
     */
    virtual std::optional<UserToken> onAccessOf(const UserTokenLogin &userTokenLogin) = 0;

    /**
     * Invalidate a token (logout).
     * @param userToken token to invalidate
     */
    virtual void onLogoutOf(const UserTokenLogin &userToken) = 0;

    /**
     * Get all active sessions of a user.
     * @param userName the username
     * @return list of active tokens of the user
     */
    virtual std::vector<UserToken> allTokensOf(const std::string &userName) = 0;

    /**
     * Revoke a session by its token id.
     * @param id the token id to revoke
     */
    virtual void revokeTokenOf(const std::string &id) = 0;
};

#endif  // CPP_BASE_LIBRARY_IAUTHSERVICE_H
