#include "base_library/features/cli/services/CommandLineService.h"

#include <date/tz.h>

#include <chrono>
#include <string>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

CommandLineService::CommandLineService(
        const std::vector<std::shared_ptr<CommandLineComponent>> &components,
        std::shared_ptr<AuthCliService> authCliService,
        std::shared_ptr<UserApi> userApi,
        std::shared_ptr<CommandLineUtils> commandLineUtils,
        std::shared_ptr<InputService> inputService,
        std::shared_ptr<TerminalService> terminalService,
        const std::shared_ptr<ProcessName> &processName)
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
    if (m_thread.joinable()) {
        m_thread.join();
    }
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

void CommandLineService::onStart() {
    auto userDtoOpt = m_authCliService->onLogin();
    if (!userDtoOpt.has_value()) {
        SignalService::raiseSignal(SIGINT);
        m_running.store(false);
        return;
    }
    m_userDto = userDtoOpt.value();
}

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
    using namespace std;
    using namespace std::chrono;
    using namespace date;
    cout << format("%FT%TZ", floor<seconds>(system_clock::now())) << " "
         << currentMenu << " % ";
    if (!m_terminalService->getLine().empty()) {
        cout << m_terminalService->getLine();
    }
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
                // Tab pressed but nothing entered => Beep for missing symbol
                if (symbolEvent.second.empty() && !tabPressed) {
                    CommandLineUtils::beep();
                    tabPressed = true;
                    printPrompt = false;
                    return;
                }
                // tab pressed with entered symbols
                if (!symbolEvent.second.empty()) {
                    std::vector<std::string> commands;
                    if (m_menu.currentOf() != nullptr) {
                        commands = m_menu.allCommandsOf();
                    } else {
                        commands = m_commandParser.allCommandsOf();
                        auto vec = m_menu.allCommandsOf();
                        for (const auto &iter: vec) {
                            commands.push_back(iter);
                        }
                    }
                    commands.erase(
                            std::remove_if(commands.begin(), commands.end(),
                                           [&symbolEvent](const std::string &command) {
                                               return !StringUtils::startsWith(
                                                       command, symbolEvent.second);
                                           }),
                            commands.end());
                    // found only one matching command => extend command
                    if (commands.size() == 1) {
                        std::string missingPart =
                                commands[0].substr(symbolEvent.second.length());
                        for (char c: missingPart) {
                            m_terminalService->onKeyPressed({KeyType::Ascii, c});
                        }
                        printPrompt = false;
                        tabPressed = false;
                        return;
                    }
                    // found no command for entered symbols => beep as error information
                    if (commands.empty()) {
                        CommandLineUtils::beep();
                        tabPressed = true;
                        printPrompt = false;
                        return;
                    }
                    // found multiple commands which matches => print all matching commands
                    // extend as much as possible
                    std::cout << "\n\t";
                    int32_t i = 0;
                    for (const auto &item: commands) {
                        i++;
                        if (i % 10 == 0) {
                            std::cout << "\n";
                        }
                        std::cout << item;
                        if (i % 10 > 0) {
                            std::cout << "\t";
                        }
                    }
                    std::cout << "\n";
                    printPrompt = true;
                    tabPressed = false;
                    return;
                }
                // Tab pressed twice with nothing entered => show helpful information
                printPrompt = true;
                tabPressed = false;
                if (m_menu.currentOf() != nullptr) {
                    m_menu.printCommandList();
                    return;
                }
                m_commandParser.printCommandList(m_menu.allMenuEntriesOf());
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
            case Symbol::CtrlC:
                onEndOfFile();
                break;
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
        if (!m_running) {
            LOG_TRACE("abort loop");
            break;
        }
    }
    LOG_TRACE("finished endless loop");
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
            std::cout << "ERROR: Invalid command '" << input
                      << "'! Please use the help function\n";
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