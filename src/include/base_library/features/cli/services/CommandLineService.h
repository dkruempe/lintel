#ifndef CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
#define CPP_BASE_LIBRARY_COMMANDLINESERVICE_H

#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/cli/models/AbstractCommandLineMenu.h"
#include "base_library/features/http/provider/ClientProvider.h"
#include "base_library/features/cli/models/CommandParser.h"

class CommandLineComponent;

class CommandLineService {
 private:
  enum Commands { CommandHelp, CommandClear, CommandMenu, CommandExit, CommandUndefined };

  AbstractCommandLineMenu m_menu;
  std::thread m_thread;
  std::atomic<bool> m_running = true;
  std::string m_clear = std::string(100, '\n');
  CommandParser<Commands, CommandUndefined> m_commandParser;

  void onComponentCommand(const std::string &input, const std::vector<std::string> &flags);

  static void onStart();

  void onHelp();

  void onPrompt();

  void run();

 public:
  explicit CommandLineService(
      const std::vector<std::shared_ptr<CommandLineComponent>> &components,
      const std::shared_ptr<Configuration> &configuration);
  ~CommandLineService();
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
