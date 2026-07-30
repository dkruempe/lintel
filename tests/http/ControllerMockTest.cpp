#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include "../mocks/MockAuthService.h"
#include "base_library/features/http/ExampleController.h"

using namespace trompeloeil;

TEST_CASE("Controller: ExampleController can be constructed with mock AuthService") {
    auto mockAuth = std::make_shared<MockAuthService>();
    REQUIRE_NOTHROW(ExampleController(mockAuth));
}

TEST_CASE("Controller: can construct ExampleController with mock AuthService") {
    auto mockAuth = std::make_shared<MockAuthService>();
    ExampleController controller(mockAuth);
}
