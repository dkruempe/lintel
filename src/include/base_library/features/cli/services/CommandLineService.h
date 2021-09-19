#ifndef CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
#define CPP_BASE_LIBRARY_COMMANDLINESERVICE_H

#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/cli/models/AbstractCommandLineMenu.h"
#include "base_library/features/cli/models/CommandParser.h"
#include "base_library/features/cli/services/AuthCliService.h"
#include "base_library/features/http/provider/ClientProvider.h"

class CommandLineComponent;

class CommandLineService {
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
  bool m_startThread = true;
  CommandParser<Commands, CommandUndefined> m_commandParser;
  std::shared_ptr<AuthCliService> m_authCliService;
  std::shared_ptr<UserApi> m_userApi;
  UserDto m_userDto;

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
      std::shared_ptr<UserApi> userApi);
  ~CommandLineService();
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
