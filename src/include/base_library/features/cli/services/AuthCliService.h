#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H

#include <memory>
#include <optional>

#include "base_library/features/http/controllers/UserApi.h"
#include "base_library/features/cli/providers/AuthArgumentProvider.h"
#include "base_library/features/cli/services/IInputService.h"
#include "base_library/features/cli/services/ITerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

class AuthCliService {
private:
    std::shared_ptr<UserApi> m_userApi;
    std::shared_ptr<CommandLineUtils> m_commandLineUtils;
    std::shared_ptr<IInputService> m_inputService;
    std::shared_ptr<ITerminalService> m_terminalService;
    std::shared_ptr<AuthArgumentProvider> m_authArgumentProvider;

public:
    AuthCliService(std::shared_ptr<UserApi> userApi,
                   std::shared_ptr<CommandLineUtils> commandLineUtils,
                   std::shared_ptr<IInputService> inputService,
                   std::shared_ptr<ITerminalService> terminalService,
                   std::shared_ptr<AuthArgumentProvider> argumentProvider);

    std::optional<UserDto> onLogin();

    void onLogout(UserDto &&userDto);

private:
    std::string readUserName();

    std::string readPassword();
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
