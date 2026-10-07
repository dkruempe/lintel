#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <utility>

#include "../mocks/MockAuthService.h"
#include "lintel/features/http/ExampleController.h"
#include "lintel/features/http/service/Controller.h"

using namespace trompeloeil;

TEST_CASE("Controller: ExampleController can be constructed with mock AuthService") {
    auto mockAuth = std::make_shared<MockAuthService>();
    REQUIRE_NOTHROW(ExampleController(mockAuth));
}

TEST_CASE("Controller: can construct ExampleController with mock AuthService") {
    auto mockAuth = std::make_shared<MockAuthService>();
    ExampleController controller(mockAuth);
}

namespace {

/** Test controller exposing the protected clientIpOf for verification */
class ClientIpProbeController : public Controller {
public:
    explicit ClientIpProbeController(std::shared_ptr<IAuthService> authService)
        : Controller(std::move(authService)) {}
    std::string resolveClientIpOf(const httplib::Request &request) {
        return clientIpOf(request);
    }
};

}  // namespace

TEST_CASE("Controller: ignores X-Forwarded-For from untrusted peer") {
    auto mockAuth = std::make_shared<MockAuthService>();
    ClientIpProbeController controller(mockAuth);
    httplib::Request request;
    request.remote_addr = "10.0.0.1";
    request.set_header("X-Forwarded-For", "203.0.113.7");
    REQUIRE(controller.resolveClientIpOf(request) == "10.0.0.1");
}

TEST_CASE("Controller: honors X-Forwarded-For for configured trusted proxies") {
    auto mockAuth = std::make_shared<MockAuthService>();
    ClientIpProbeController controller(mockAuth);
    controller.setTrustedProxies({"10.0.0.0/8"});
    httplib::Request request;
    request.remote_addr = "10.0.0.1";
    request.set_header("X-Forwarded-For", "203.0.113.7");
    REQUIRE(controller.resolveClientIpOf(request) == "203.0.113.7");
}
