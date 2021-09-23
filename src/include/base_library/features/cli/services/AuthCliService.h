#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H

#include <memory>
#include <optional>

#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/cli/services/InputService.h"
#include "base_library/features/cli/services/TerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

class AuthCliService {
 private:
  std::shared_ptr<UserApi> m_userApi;
  std::shared_ptr<CommandLineUtils> m_commandLineUtils;
  InputService m_inputService;
  TerminalService m_terminalService;

  std::string readPassword();

 public:
  explicit AuthCliService(std::shared_ptr<UserApi> userApi,
                          std::shared_ptr<CommandLineUtils> commandLineUtils);
  UserDto onLogin();
  void onLogout(UserDto &&userDto);
  void setTerminalService(const TerminalService &terminalService);
  void setInputService(const InputService &inputService);
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
