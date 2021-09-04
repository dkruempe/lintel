#include "base_library/features/base/controller/UserController.h"

#include <utility>

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
void UserController::loginOfPost(const httplib::Request& request,
                                 httplib::Response& response,
                                 const ContentType& contentType) {
  try {
    UserLoginDto userLoginDto;
    userLoginDto.JsonSerializable::deserialize(request.body);
    UserLogin userLogin{request.remote_addr, userLoginDto.getUserName(),
                        userLoginDto.getPassword()};
    LOG_INFO("login {} {} {}", userLoginDto.getUserName(),
             userLoginDto.getPassword(), request.remote_addr);
    std::optional<UserToken> userToken = m_authService->onLoginOf(userLogin);
    if (!userToken.has_value()) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      return;
    }
    LOG_INFO("{}-{}: successfull login", userToken.value().m_ipAddress,
             userToken.value().m_id);
    UserDto userDto(userToken.value().m_user, userToken.value().m_id);
    const std::string responseBody = userDto.JsonSerializable::serialize();
    response.set_content(responseBody, "application/json");
  } catch (const std::exception& exception) {
    LOG_ERROR("login failed {} for {}", exception.what(), request.body);
  }
}
UserController::UserController(std::shared_ptr<AuthService> authService)
    : Controller(), m_authService(std::move(authService)) {}
