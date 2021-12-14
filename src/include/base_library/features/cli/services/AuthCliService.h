#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H

#include <memory>
#include <optional>

#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/cli/providers/AuthArgumentProvider.h"
#include "base_library/features/cli/services/InputService.h"
#include "base_library/features/cli/services/TerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

class AuthCliService {
 private:
  std::shared_ptr<UserApi> m_userApi;
  std::shared_ptr<CommandLineUtils> m_commandLineUtils;
  std::shared_ptr<InputService> m_inputService;
  std::shared_ptr<TerminalService> m_terminalService;
  std::shared_ptr<AuthArgumentProvider> m_authArgumentProvider;

  static std::string readPassword();

 public:
  AuthCliService(std::shared_ptr<UserApi> userApi,
                 std::shared_ptr<CommandLineUtils> commandLineUtils,
                 std::shared_ptr<InputService> inputService,
                 std::shared_ptr<TerminalService> terminalService,
                 std::shared_ptr<AuthArgumentProvider> argumentProvider);
  UserDto onLogin();
  void onLogout(UserDto &&userDto);
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
