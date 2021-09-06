#include "base_library/features/cli/services/CommandLineService.h"
#define FMT_HEADER_ONLY
#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <string>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/cli/models/CommandLineComponent.h"

CommandLineService::CommandLineService(
    const std::vector<std::shared_ptr<CommandLineComponent>> &components,
    const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<AuthCliService> authCliService)
    : m_menu(components),
      m_thread([&]() { run(); }),
      m_authCliService(std::move(authCliService)) {
  m_commandParser.addCommand(
      Command("help",
              "Show all available commands in the current selected menu."),
      CommandHelp);
  m_commandParser.addCommand(Command("menu", "Show all available menus"),
                             CommandMenu);
  m_commandParser.addCommand(
      Command("exit", "Exit of current menu or whole cli"), CommandExit);
  m_commandParser.addCommand(Command("clear", "Clear cli"), CommandClear);
}

CommandLineService::~CommandLineService() {
  m_running.store(false);
  LOG_TRACE("finish command line service");
  m_thread.join();
  LOG_INFO("stopped command line service");
}
void CommandLineService::onComponentCommand(
    const std::string &input, const std::vector<std::string> &flags) {
  Commands command = m_commandParser.parse(input, flags);
  if (command == CommandUndefined) {
    m_menu.onCommand(input, flags);
    return;
  }
  switch (command) {
    case CommandHelp:
      m_menu.onHelp();
      break;
    case CommandMenu:
      m_menu.onShowMenu();
      break;
    case CommandExit:
      m_menu.onExit();
      break;
    case CommandClear:
      std::cout << m_clear;
      break;
    default:
      break;
  }
}
void CommandLineService::onStart() {
  m_userDto = m_authCliService->onLogin();
  std::string startInformation =
      "Welcome to the Command Line Interface:\n"
      "Try >help< for a list of commands\n"
      "Try >exit< to go back or exit the Command Line Interface\n";
  fmt::print(startInformation);
}
void CommandLineService::onHelp() {
  m_commandParser.printHelp(
      "MAIN", "",
      "This is the base entering point for the whole cli service. Here you can "
      "list all available menus and select your prefered one!");
}
void CommandLineService::onPrompt() {
  // timestamp MENU %
  std::string_view currentMenu =
      m_menu.currentOf() == nullptr ? "MAIN" : m_menu.currentOf()->getName();
  std::chrono::system_clock::time_point point =
      std::chrono::system_clock::now();
  fmt::print("{:%Y-%m-%d %H:%M:%S} {} % ", point, currentMenu);
}
void CommandLineService::run() {
  onStart();
  while (m_running) {
    onPrompt();
    std::string temp;
    std::getline(std::cin, temp);
    std::vector<std::string> flags = StringUtils::split(temp, ' ');
    std::string input;
    if (!flags.empty()) {
      input = flags[0];
      flags.erase(flags.begin());
    }
    if (m_menu.currentOf() != nullptr) {
      onComponentCommand(input, flags);
      continue;
    }
    Commands command = m_commandParser.parse(input, flags);
    if (command == CommandUndefined) {
      if (input.empty()) {
        continue;
      }
      if (!m_menu.onMenu(input)) {
        fmt::print(
            "ERROR: Invalid command '{}'! Please use the help function\n",
            input);@k
        continue;
      }
    }
    switch (command) {
      case CommandExit:
        m_authCliService->onLogout(std::move(m_userDto));
        SignalService::raiseSignal(SIGINT);
        m_running.store(false);
        break;
      case CommandHelp:
        onHelp();
        break;
      case CommandClear:
        std::cout << m_clear;
        break;
      case CommandMenu:
        m_menu.onShowMenu();
        break;
      default:
        break;
    }
  }
}