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

class CommandLineComponent;

class CommandLineService {
 private:
  AbstractCommandLineMenu m_menu;
  std::thread m_thread;
  std::atomic<bool> m_running = true;
  std::string m_clear = std::string(100, '\n');

  enum Command { CommandHelp, CommandClear, CommandMenu, CommandExit };

  std::map<std::string_view, Command> m_commands = {
      {"?", CommandHelp},    {"help", CommandHelp}, {"m", CommandMenu},
      {"menu", CommandMenu}, {"e", CommandExit},    {"exit", CommandExit},
      {"clear", CommandClear}};

  void onComponentCommand(const std::string &command, const std::vector<std::string> &parameters);

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
