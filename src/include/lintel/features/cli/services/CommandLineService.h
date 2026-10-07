#ifndef LINTEL_COMMANDLINESERVICE_H
#define LINTEL_COMMANDLINESERVICE_H

#include "CommandLineHistoryService.h"


#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "lintel/core/services/AbstractService.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/models/AbstractCommandLineMenu.h"
#include "lintel/features/cli/models/CommandParser.h"
#include "lintel/features/cli/services/AuthCliService.h"
#include "lintel/features/cli/services/IInputService.h"
#include "lintel/features/cli/services/ITerminalService.h"
#include "lintel/features/cli/utils/CommandLineUtils.h"
#include "lintel/features/http/provider/ClientProvider.h"

class CommandLineComponent;

/** Main service that drives the interactive command-line interface */
class CommandLineService : public AbstractService<CommandLineService>
{
private:
  enum Commands { CommandHelp, CommandClear, CommandMenu, CommandHistory, CommandExit, CommandUndefined };

  AbstractCommandLineMenu m_menu;
  std::thread m_thread;
  std::atomic<bool> m_running = true;
  CommandParser<Commands, CommandUndefined> m_commandParser;
  std::shared_ptr<AuthCliService> m_authCliService;
  std::shared_ptr<UserApi> m_userApi;
  std::shared_ptr<CommandLineUtils> m_commandLineUtils;
  UserDto m_userDto;
  std::shared_ptr<IInputService> m_inputService;
  std::shared_ptr<ITerminalService> m_terminalService;
  std::shared_ptr<CommandLineHistoryService> m_commandLineHistoryService;
  std::optional<std::string> m_helpComponentName;

  /** @return name of the current menu */
  std::string menuNameOf();

  /** Route a command to the active component */
  void onComponentCommand(const std::string &input, const std::vector<std::string> &flags);

  /** Handle a top-level command */
  void onCommand(const std::string &input, const std::vector<std::string> &flags);

  /** Handle end-of-file signal */
  void onEndOfFile();

  /** Called when the service starts */
  void onStart();

  /** Show the help screen */
  void onHelp();

  /** Display the input prompt */
  void onPrompt();

  /** Handle a regular command input symbol */
  void handleCommandInput(const SymbolEvent &symbolEvent, bool &tabPressed, bool &printPrompt);

  /** Handle tab key input for autocomplete */
  void handleTabInput(const SymbolEvent &symbolEvent, bool &tabPressed, bool &printPrompt);

  /** Main CLI loop running in a background thread */
  void run();

public:
  explicit CommandLineService(const std::vector<std::shared_ptr<CommandLineComponent>> &components,
    std::shared_ptr<AuthCliService> authCliService,
    std::shared_ptr<UserApi> userApi,
    std::shared_ptr<CommandLineUtils> commandLineUtils,
    std::shared_ptr<IInputService> inputService,
    std::shared_ptr<ITerminalService> terminalService,
    const std::shared_ptr<ProcessName> &processName,
    std::shared_ptr<CommandLineHistoryService> commandLineHistoryService);

  virtual ~CommandLineService();

  void onInitialize() override;

  void onShutdown() override;
};

#endif// LINTEL_COMMANDLINESERVICE_H
