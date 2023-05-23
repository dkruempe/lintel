#include "base_library/features/http/ExampleController.h"

#include <utility>

ExampleController::ExampleController(std::shared_ptr<AuthService> authService)
        : Controller(std::move(authService)) {}

void ExampleController::helloGet(const httplib::Request &request,
                                 httplib::Response &response,
                                 const ContentType &contentType,
                                 const std::optional<UserToken> &user) {
    if (user.has_value()) {
        response.set_content("Hello >" + user->m_user.getUserName() + "< !",
                             "text/plain");
    } else {
        response.set_content("Hello World!", "text/plain");
    }
}