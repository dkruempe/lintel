#include "base_library/features/cli/services/AuthCliService.h"

#include <utility>

#include "base_library/features/cli/utils/CommandLineUtils.h"

AuthCliService::AuthCliService(std::shared_ptr<UserApi> userApi)
    : m_userApi(std::move(userApi)) {}
UserDto AuthCliService::onLogin() {
  bool success = false;
  std::optional<UserDto> optUserDto;
  while (!success) {
    std::cout << "Please enter the username: ";
    std::string userName;
    std::string password;
    std::cin >> userName;
    password = readPassword();
    UserLoginDto userLoginDto(userName, password);
    optUserDto = m_userApi->loginOf(userLoginDto);
    if (optUserDto.has_value()) {
      success = true;
    }
    CommandLineUtils::clear();
  }
  return std::move(optUserDto.value());
}
void AuthCliService::onLogout(UserDto &&userDto) {
  UserTokenDto userTokenDto(userDto.getId());
  bool success = m_userApi->logoutOf(userTokenDto);
  if (!success) {
    std::cout << "ERROR: something went wrong during logout with id >"
              << userDto.getUserName() << "<\n";
    return;
  }
  std::cout << "INFO: succesfull logout of >" << userDto.getUserName() << "<\n";
}

std::string AuthCliService::readPassword() {
  std::cout << "Please enter the password: ";
  std::string password;
  CommandLineUtils::disableOfInputEcho();
  std::cin >> password;
  CommandLineUtils::enableOfInputEcho();
  return password;
}
