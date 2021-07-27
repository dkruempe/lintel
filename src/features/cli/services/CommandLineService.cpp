#include "base_library/features/cli/services/CommandLineService.h"
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <chrono>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"
#include "base_library/features/cli/models/CommandLineComponent.h"

CommandLineService::CommandLineService(
    const std::vector<std::shared_ptr<CommandLineComponent>> &components,
    const std::shared_ptr<Configuration> &configuration)
    : m_menu(components), m_thread([&]() { run(); }) {}

CommandLineService::~CommandLineService() {
  m_running.store(false);
  LOG_TRACE("finish command line service");
  m_thread.join();
  LOG_INFO("stopped command line service");
}
void CommandLineService::onComponentCommand(const std::string &command) {
  auto found = m_commands.find(command);
  if (found == m_commands.end()) {
    m_menu.onCommand(command);
    return;
  }
  switch (found->second) {
    case CommandHelp:
      m_menu.onHelp();
      break;
    case CommandMenu:
      m_menu.onShowMenu();
      break;
    case CommandExit:
      m_menu.onExit();
      break;
  }
}
void CommandLineService::onStart() {
  std::string startInformation =
      "Welcome to the Command Line Interface:\n"
      "Try >help< or >?< for a list of commands\n"
      "Try >menue< or >m< to work with the menue system\n"
      "Try >exit< or >e< to go back or exit the Command Line Interface\n";
  fmt::print(startInformation);
}
void CommandLineService::onHelp() {
  // group commands map by command enum
  std::map<Command, std::vector<std::string_view>> map;
  for (auto &iter : m_commands) {
    auto found = map.find(iter.second);
    if (found == map.end()) {
      map.insert({iter.second, {iter.first}});
    } else {
      found->second.push_back(iter.first);
    }
  }

  std::function<void(std::vector<std::string_view> &)> print =
      [&](std::vector<std::string_view> &aliases) {
        fmt::print("[");
        for (std::size_t i = 0; i < aliases.size(); i++) {
          fmt::print("{}", aliases[i]);
          if (i < aliases.size() - 1) {
            fmt::print(", ", aliases[i]);
          }
        }
        fmt::print("]\n");
      };

  fmt::print("Help Overview\n");
  for (auto &[command, aliases] : map) {
    switch (command) {
      case CommandMenu:
        fmt::print("COMMAND_MENU: Shows available Menu entries");
        print(aliases);
        break;
      case CommandExit:
        fmt::print("COMMAND_EXIT: Exits current Menu or total CLI itself");
        print(aliases);
        break;
      case CommandHelp:
        fmt::print(
            "COMMAND_HELP: Shows all available commands in current menu");
        print(aliases);
        break;
    }
  }
}
void CommandLineService::onPrompt() {
  // timestamp MENU %
  std::string_view currentMenu =
      m_menu.currentOf() == nullptr ? "MAIN" : m_menu.currentOf()->getName();
  fmt::print("{} % ", currentMenu);
}
void CommandLineService::run() {
  onStart();
  while (m_running) {
    onPrompt();
    std::string temp;
    std::getline(std::cin, temp);
    auto found = m_commands.find(temp);
    if (m_menu.currentOf() != nullptr) {
      onComponentCommand(temp);
      continue;
    }
    if (found == m_commands.end()) {
      bool success = m_menu.onMenu(temp);
      if (!success) {
        fmt::print(
            "ERROR: Invalid command '{}'! Please use the help function\n",
            temp);
      }
      continue;
    }
    switch (found->second) {
      case CommandExit:
        SignalService::raiseSignal(SIGINT);
        m_running.store(false);
        break;
      case CommandHelp:
        onHelp();
        break;
      case CommandMenu:
        m_menu.onShowMenu();
        break;
    }
  }
}