#ifndef CPP_BASE_LIBRARY_MOCKUSERAPI_H
#define CPP_BASE_LIBRARY_MOCKUSERAPI_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/http/controllers/UserApi.h"

class MockUserApi : public UserApi {
public:
    MAKE_MOCK1(loginOf, std::optional<UserDto>(const UserLoginDto &), override);
    MAKE_MOCK1(logoutOf, bool(const UserTokenDto &), override);
    MAKE_MOCK0(isLoggedIn, bool(), override);
};

#endif
