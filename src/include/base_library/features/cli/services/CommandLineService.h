#ifndef CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
#define CPP_BASE_LIBRARY_COMMANDLINESERVICE_H

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/cli/models/AbstractCommandLineMenu.h"
#include "base_library/features/cli/models/CommandParser.h"
#include "base_library/features/cli/services/AuthCliService.h"
#include "base_library/features/cli/services/InputService.h"
#include "base_library/features/cli/services/TerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"
#include "base_library/features/http/provider/ClientProvider.h"

class CommandLineComponent;

class CommandLineService : public AbstractService<CommandLineService> {
private:
  enum Commands {
    CommandHelp,
    CommandClear,
    CommandMenu,
    CommandExit,
    CommandUndefined
  };

  AbstractCommandLineMenu m_menu;
  std::thread m_thread;
  std::atomic<bool> m_running = true;
  CommandParser<Commands, CommandUndefined> m_commandParser;
  std::shared_ptr<AuthCliService> m_authCliService;
  std::shared_ptr<UserApi> m_userApi;
  std::shared_ptr<CommandLineUtils> m_commandLineUtils;
  UserDto m_userDto;
  std::shared_ptr<InputService> m_inputService;
  std::shared_ptr<TerminalService> m_terminalService;
  std::optional<std::string> m_helpComponentName;

  void onComponentCommand(const std::string &input,
                          const std::vector<std::string> &flags);

  void onCommand(const std::string &input,
                 const std::vector<std::string> &flags);

  void onEndOfFile();

  void onStart();

  void onHelp();

  void onPrompt();

  void run();

public:
  explicit CommandLineService(
      const std::vector<std::shared_ptr<CommandLineComponent>> &components,
      const std::shared_ptr<Configuration> &configuration,
      std::shared_ptr<AuthCliService> authCliService,
      std::shared_ptr<UserApi> userApi,
      std::shared_ptr<CommandLineUtils> commandLineUtils,
      std::shared_ptr<InputService> inputService,
      std::shared_ptr<TerminalService> terminalService,
      std::shared_ptr<ProcessName> processName);
  virtual ~CommandLineService();

  void onInitialize() override;
};

#endif // CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
