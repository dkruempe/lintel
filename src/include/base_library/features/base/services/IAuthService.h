#ifndef CPP_BASE_LIBRARY_IAUTHSERVICE_H
#define CPP_BASE_LIBRARY_IAUTHSERVICE_H

#include <date/tz.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "base_library/features/base/models/User.h"

struct UserLogin {
    std::string m_ipAddress;
    std::string m_userName;
    std::string m_password;
};

struct UserTokenLogin {
    std::string m_ipAddress;
    std::string m_id;
};

struct UserToken {
    std::string m_ipAddress;
    std::string m_id;
    date::sys_time<std::chrono::microseconds> m_lastAccessTimestamps;
    User m_user;
};

class IAuthService {
public:
    virtual ~IAuthService() = default;

    virtual std::optional<UserToken> onLoginOf(const UserLogin &userLogin) = 0;

    virtual std::optional<UserToken> onAccessOf(const UserTokenLogin &userTokenLogin) = 0;

    virtual void onLogoutOf(const UserTokenLogin &userToken) = 0;
};

#endif  // CPP_BASE_LIBRARY_IAUTHSERVICE_H
