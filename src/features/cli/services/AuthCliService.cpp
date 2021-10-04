#include "base_library/features/cli/services/AuthCliService.h"

#include <utility>

#include "base_library/features/cli/utils/CommandLineUtils.h"

AuthCliService::AuthCliService(
    std::shared_ptr<UserApi> userApi,
    std::shared_ptr<CommandLineUtils> commandLineUtils,
    std::shared_ptr<InputService> inputService,
    std::shared_ptr<TerminalService> terminalService,
    std::shared_ptr<AuthArgumentProvider> authArgumentProvider)
    : m_userApi(std::move(userApi)),
      m_commandLineUtils(std::move(commandLineUtils)),
      m_inputService(std::move(inputService)),
      m_terminalService(std::move(terminalService)),
      m_authArgumentProvider(std::move(authArgumentProvider)) {}
UserDto AuthCliService::onLogin() {
  bool success = false;
  std::optional<UserDto> optUserDto;
  std::string userName;
  auto optUserName = m_authArgumentProvider->getUserName();
  if (optUserName.has_value()) {
    userName = optUserName.value();
    LOG_TRACE("set user_name {}", userName);
  }
  while (!success) {
    if (!optUserName.has_value()) {
      userName.clear();
    }
    std::string password;
    if (userName.empty()) {
      std::cout << "Please enter the username: ";
      Symbol event = Symbol::Nothing;
      while (event != Symbol::Command) {
        KeyEvent keyPressed = m_inputService->onRead();
        SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed);
        event = symbolEvent.first;
        if (event == Symbol::Command) {
          userName = symbolEvent.second;
        }
      }
    }
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
  std::cin >> password;
  return password;
}
