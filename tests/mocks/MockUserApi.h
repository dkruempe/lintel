#ifndef CPP_BASE_LIBRARY_MOCKUSERAPI_H
#define CPP_BASE_LIBRARY_MOCKUSERAPI_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/http/controllers/UserApi.h"

class MockUserApi : public UserApi {
public:
    MAKE_MOCK1(loginOf, std::optional<UserDto>(const UserLoginDto &), override);
    MAKE_MOCK1(logoutOf, bool(const UserTokenDto &), override);
    MAKE_MOCK0(isLoggedIn, bool(), override);
    MAKE_MOCK0(allOf, std::vector<GroupDto>(), override);
    MAKE_MOCK1(allOf, std::vector<GroupDto>(const std::string &), override);
    MAKE_MOCK2(allOf, std::vector<GroupDto>(const std::string &, bool), override);
    MAKE_MOCK1(allOf, std::vector<GroupDto>(bool), override);
    MAKE_MOCK0(allUsersOf, std::vector<UserDto>(), override);
    MAKE_MOCK1(allUsersOf, std::vector<UserDto>(const std::string &), override);
    MAKE_MOCK1(createOf, void(const UserDto &), override);
    MAKE_MOCK3(updateOf, void(const std::string &, const std::set<std::string> &, const std::set<std::string> &), override);
    MAKE_MOCK1(deleteOf, void(const std::vector<std::string> &), override);
};

#endif
