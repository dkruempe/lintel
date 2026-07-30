#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H

#include <memory>
#include <optional>

#include "base_library/features/http/controllers/UserApi.h"
#include "base_library/features/cli/providers/AuthArgumentProvider.h"
#include "base_library/features/cli/services/IInputService.h"
#include "base_library/features/cli/services/ITerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

/** Handles CLI-based user authentication (login/logout) */
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

    /** Prompt the user for credentials and attempt login */
    std::optional<UserDto> onLogin();

    /** Log out the given user */
    void onLogout(UserDto &&userDto);

private:
    /** Read username from terminal input */
    std::string readUserName();

    /** Read password from terminal input with character hiding */
    std::string readPassword();
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
