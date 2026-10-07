#include "lintel/features/cli/services/AuthCliService.h"

#include <lintel/core/services/SignalService.h>

#include <utility>

#include "lintel/features/cli/utils/CommandLineUtils.h"

AuthCliService::AuthCliService(
        std::shared_ptr<UserApi> userApi,
        std::shared_ptr<CommandLineUtils> commandLineUtils,
        std::shared_ptr<IInputService> inputService,
        std::shared_ptr<ITerminalService> terminalService,
        std::shared_ptr<AuthArgumentProvider> authArgumentProvider)
        : m_userApi(std::move(userApi)),
          m_commandLineUtils(std::move(commandLineUtils)),
          m_inputService(std::move(inputService)),
          m_terminalService(std::move(terminalService)),
          m_authArgumentProvider(std::move(authArgumentProvider)) {}

std::string AuthCliService::readUserName() {
    std::string userName;
    std::cout << "Please enter the username: ";
    Symbol event = Symbol::Nothing;
    while (event != Symbol::Command && userName.empty()) {
        KeyEvent keyPressed = m_inputService->onRead();
        SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed, "Login");
        event = symbolEvent.first;
        switch (event) {
            case Symbol::CtrlC:
            case Symbol::Eof: {
                CommandLineUtils::clear();
                return {};
            }
            case Symbol::Command: {
                userName = symbolEvent.second;
                break;
            }
            default:
                break;
        }
        if (event == Symbol::Command) {
            userName = symbolEvent.second;
        }
    }
    return userName;
}

std::string AuthCliService::readPassword() {
    std::string password;
    std::cout << "Please enter Password : ";
    m_terminalService->resetCursor();
    m_terminalService->enableHideChars();
    Symbol event = Symbol::Nothing;
    while (event != Symbol::Command && password.empty()) {
        KeyEvent keyPressed = m_inputService->onRead();
        SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed, "Login");
        event = symbolEvent.first;
        switch (event) {
            case Symbol::CtrlC:
            case Symbol::Eof: {
                m_terminalService->disableHideChars();
                CommandLineUtils::clear();
                return {};
            }
            case Symbol::Command: {
                password = symbolEvent.second;
                break;
            }
            default:
                break;
        }
    }
    m_terminalService->disableHideChars();
    return password;
}

std::optional<UserDto> AuthCliService::onLogin() {
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
            userName = readUserName();
            if (userName.empty()) {
                return std::nullopt;
            }
        }
        if (!userName.empty()) {
            password = readPassword();
            if (password.empty()) {
                continue;
            }
            UserLoginDto userLoginDto(userName, password);
            optUserDto = m_userApi->loginOf(userLoginDto);
            if (optUserDto.has_value()) {
                success = true;
            }
        }
        CommandLineUtils::clear();
    }
    if (optUserDto.has_value()) {
        return std::make_optional(std::move(optUserDto.value()));
    }
    return std::nullopt;
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
