#include "base_library/features/cli/services/AuthCliService.h"

#include <base_library/core/services/SignalService.h>

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
  std::string password;
  auto optUserName = m_authArgumentProvider->getUserName();
  if (optUserName.has_value()) {
    userName = optUserName.value();
    LOG_TRACE("set user_name {}", userName);
  }

  while (!success) {
    if (!optUserName.has_value()) {
      userName.clear();
    }
    password.clear();
    if (userName.empty()) {
      std::cout << "Please enter the username: ";
      Symbol event = Symbol::Nothing;
      while (event != Symbol::Command && userName.empty()) {
        KeyEvent keyPressed = m_inputService->onRead();
        SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed);
        event = symbolEvent.first;
        if (event == Symbol::Command) {
          userName = symbolEvent.second;
        }
      }
    }
    if (!userName.empty()) {
      Symbol event = Symbol::Nothing;
      std::cout << "Please enter Password : ";
      m_terminalService->resetCursor();
      m_terminalService->enableHideChars();
      while (event != Symbol::Command && password.empty()) {
        KeyEvent keyPressed = m_inputService->onRead();
        SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed);
        event = symbolEvent.first;
        if (event == Symbol::Command) {
          password = symbolEvent.second;
        }
      }
      if (password.empty()) {
        continue;
      }
      m_terminalService->disableHideChars();
      UserLoginDto userLoginDto(userName, password);
      optUserDto = m_userApi->loginOf(userLoginDto);
      if (optUserDto.has_value()) {
        success = true;
      }
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
  std::string tempPassword;
  std::cin >> tempPassword;
  std::string password;
  for (auto &iter : tempPassword) {
    if (iter == 127) {
      LOG_TRACE("pressed backspace during password read");
      password = std::string(password.begin(), password.end() - 1);
      continue;
    }
    password += iter;
  }
  return password;
}
