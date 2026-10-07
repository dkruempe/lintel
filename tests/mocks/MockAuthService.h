#ifndef LINTEL_MOCKAUTHSERVICE_H
#define LINTEL_MOCKAUTHSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "lintel/features/base/services/IAuthService.h"

class MockAuthService : public IAuthService {
public:
    MAKE_MOCK1(onLoginOf, std::optional<UserToken>(const UserLogin &), override);
    MAKE_MOCK1(onAccessOf, std::optional<UserToken>(const UserTokenLogin &), override);
    MAKE_MOCK1(onLogoutOf, void(const UserTokenLogin &), override);
    MAKE_MOCK1(allTokensOf, std::vector<UserToken>(const std::string &), override);
    MAKE_MOCK1(revokeTokenOf, void(const std::string &), override);
};

#endif  // LINTEL_MOCKAUTHSERVICE_H
