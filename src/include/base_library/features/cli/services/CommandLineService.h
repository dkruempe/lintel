#ifndef CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
#define CPP_BASE_LIBRARY_COMMANDLINESERVICE_H

#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "base_library/features/cli/models/AbstractCommandLineMenu.h"

class CommandLineComponent;

class CommandLineService {
 private:
  AbstractCommandLineMenu m_menu;
  std::thread m_thread;
  std::atomic<bool> m_running = true;

  enum COMMAND { COMMAND_HELP, COMMAND_MENU, COMMAND_EXIT };

  std::map<std::string_view, COMMAND> m_commands = {
      {"?", COMMAND_HELP},    {"help", COMMAND_HELP}, {"m", COMMAND_MENU},
      {"menu", COMMAND_MENU}, {"e", COMMAND_EXIT},    {"exit", COMMAND_EXIT}};

  void onComponentCommand(const std::string& command);

  void onStart();

  void onHelp();

  void onPrompt();

  void run();

 public:
  explicit CommandLineService(
      const std::vector<std::shared_ptr<CommandLineComponent>>& components);
  ~CommandLineService();
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINESERVICE_H
