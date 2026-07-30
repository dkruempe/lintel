#ifndef CPP_BASE_LIBRARY_MOCKAUTHSERVICE_H
#define CPP_BASE_LIBRARY_MOCKAUTHSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/base/services/IAuthService.h"

class MockAuthService : public IAuthService {
public:
    MAKE_MOCK1(onLoginOf, std::optional<UserToken>(const UserLogin &), override);
    MAKE_MOCK1(onAccessOf, std::optional<UserToken>(const UserTokenLogin &), override);
    MAKE_MOCK1(onLogoutOf, void(const UserTokenLogin &), override);
};

#endif  // CPP_BASE_LIBRARY_MOCKAUTHSERVICE_H
