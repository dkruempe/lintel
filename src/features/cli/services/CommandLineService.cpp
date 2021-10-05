#include "base_library/features/cli/services/CommandLineService.h"
#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <string>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

CommandLineService::CommandLineService(
    const std::vector<std::shared_ptr<CommandLineComponent>> &components,
    const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<AuthCliService> authCliService,
    std::shared_ptr<UserApi> userApi,
    std::shared_ptr<CommandLineUtils> commandLineUtils,
    std::shared_ptr<InputService> inputService,
    std::shared_ptr<TerminalService> terminalService,
    std::shared_ptr<ProcessName> processName)
    : AbstractService<CommandLineService>(processName->getProcessName()),
      m_menu(components),
      m_authCliService(std::move(authCliService)),
      m_userApi(std::move(userApi)),
      m_commandLineUtils(std::move(commandLineUtils)),
      m_inputService(std::move(inputService)),
      m_terminalService(std::move(terminalService)) {
  m_commandParser.addCommand(
      Command("help",
              "Show all available commands in the current selected menu.")
          .addArgument({"--component", "-c"}, &m_helpComponentName,
                       "component name", true),
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
    m_menu.onCommand(m_userDto, input, flags);
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
      CommandLineUtils::clear();
      break;
    default:
      break;
  }
}
void CommandLineService::onStart() { m_userDto = m_authCliService->onLogin(); }
void CommandLineService::onHelp() {
  m_commandParser.printHelp(
      "MAIN", "",
      "This is the base entering point for the whole cli service. Here you can "
      "list all available menus and select your preferred one!");
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
  bool printPrompt = true;
  bool tabPressed = true;
  std::function<void(const SymbolEvent &symbolEvent)> handleCommand =
      [&](const SymbolEvent &symbolEvent) {
        tabPressed = false;
        printPrompt = true;
        std::string temp = symbolEvent.second;
        bool isLoggedIn = m_userApi->isLoggedIn();
        if (!isLoggedIn) {
          std::cout
              << "WARNING: timeout of login server wise => shutdown cli\n";
          SignalService::raiseSignal(SIGINT);
          m_running.store(false);
          return;
        }
        // III handle EOF signal
        if (std::cin.eof()) {
          onEndOfFile();
          return;
        }
        // IV try to execute command
        LOG_TRACE("received input >{}<", temp);
        std::vector<std::string> flags = StringUtils::split(temp, ' ');
        std::string input;
        if (!flags.empty()) {
          input = flags[0];
          flags.erase(flags.begin());
        }
        if (m_menu.currentOf() != nullptr) {
          onComponentCommand(input, flags);
          return;
        }
        onCommand(input, flags);
      };
  std::function<void(const SymbolEvent &symbolEvent)> handleTab =
      [&](const SymbolEvent &symbolEvent) {
        if (symbolEvent.second.empty() && !tabPressed) {
          CommandLineUtils::beep();
          tabPressed = true;
          printPrompt = false;
          return;
        }
        if (!symbolEvent.second.empty()) {
          return;
        }
        printPrompt = true;
        tabPressed = false;
        if (m_menu.currentOf() != nullptr) {
          m_menu.printCommandList();
          return;
        }
        m_commandParser.printCommandList();
        return;
      };
  while (m_running) {
    // I prompt
    if (printPrompt) {
      onPrompt();
    }
    // II read input
    KeyEvent keyPressed = m_inputService->onRead();
    SymbolEvent symbolEvent = m_terminalService->onKeyPressed(keyPressed);
    switch (symbolEvent.first) {
      case Symbol::Tab:
        handleTab(symbolEvent);
        break;
      case Symbol::Command:
        handleCommand(symbolEvent);
        break;
      case Symbol::Eof: {
        tabPressed = false;
        printPrompt = false;
        onEndOfFile();
        break;
      }
      default: {
        tabPressed = false;
        printPrompt = false;
        break;
      }
    }
  }
}

void CommandLineService::onEndOfFile() {
  m_authCliService->onLogout(std::move(m_userDto));
  SignalService::raiseSignal(SIGINT);
  m_running.store(false);
}

void CommandLineService::onCommand(const std::string &input,
                                   const std::vector<std::string> &flags) {
  Commands command = m_commandParser.parse(input, flags);
  if (command == CommandUndefined) {
    if (input.empty()) {
      return;
    }
    if (!m_menu.onMenu(input)) {
      fmt::print("ERROR: Invalid command '{}'! Please use the help function\n",
                 input);
      return;
    }
  }
  switch (command) {
    case CommandExit:
      m_authCliService->onLogout(std::move(m_userDto));
      SignalService::raiseSignal(SIGINT);
      m_running.store(false);
      break;
    case CommandHelp:
      if (m_helpComponentName.has_value()) {
        m_commandParser.printHelp(m_helpComponentName.value());
        break;
      }
      onHelp();
      break;
    case CommandClear:
      CommandLineUtils::clear();
      break;
    case CommandMenu:
      m_menu.onShowMenu();
      break;
    default:
      break;
  }
}
void CommandLineService::onInitialize() {
  m_thread = std::thread([&]() { run(); });
}