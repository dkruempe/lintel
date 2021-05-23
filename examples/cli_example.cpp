#include <base_library/core/services/LoggerService.h>
#include <base_library/core/services/SignalService.h>
#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/Cryption.h>
#include <base_library/features/websocket/configuration/WebsocketComponent.h>
#include <base_library/features/websocket/controller/EchoController.h>
#include <base_library/features/websocket/controller/EchoRequest.h>
#include <base_library/features/websocket/controller/EchoResponse.h>
#include <base_library/features/websocket/services/Client.h>
#include <fmt/color.h>
#include <fmt/core.h>

#include <atomic>
#include <csignal>
#include <functional>
#include <iostream>
#include <magic_enum.hpp>
#include <memory>
#include <thread>
#include <utility>
class CommandLineComponent {
 private:
  std::string name;
  std::string alias;

 public:
  CommandLineComponent(std::string name, std::string alias)
      : name(std::move(name)), alias(std::move(alias)) {}

  std::string getName() { return name; }

  std::string getAlias() { return alias; }

  virtual void onCommand(const std::string &input) = 0;

  virtual void onHelp() = 0;

  virtual void onShowMenu() = 0;

  virtual bool onMenu(const std::string &component) = 0;

  virtual bool onExit() = 0;
};

class ConfigurationComponent : public CommandLineComponent {
 private:
  enum STATE { ENCRYPT, DECRYPT, SHOW, UNDEFINED };
  STATE currentState = UNDEFINED;
  Cryption cryption;
  std::string name;

 public:
  ConfigurationComponent() : CommandLineComponent("Configuration", "conf") {}

  void onCommand(const std::string &input) override {
    switch (currentState) {
      case ENCRYPT:
        fmt::print("{}\n", cryption.encryption(input));
        currentState = UNDEFINED;
        return;
      case DECRYPT:
        fmt::print("{}\n", cryption.decryption(input));
        currentState = UNDEFINED;
        return;
      default:
        break;
    }
    STATE state = magic_enum::enum_cast<STATE>(input).value_or(UNDEFINED);
    switch (state) {
      case ENCRYPT:
        fmt::print("Encryption of: \n");
        currentState = ENCRYPT;
        break;
      case DECRYPT:
        fmt::print("Decryption of: \n");
        currentState = DECRYPT;
        break;
      case SHOW:
        currentState = UNDEFINED;
        break;
      case UNDEFINED:
        fmt::print("Wrong command \n");
        currentState = UNDEFINED;
        break;
    }
  }

  void onShowMenu() override { fmt::print("No submenu available!"); }

  bool onMenu(const std::string &component) override { return true; }

  void onHelp() override {
    auto values = magic_enum::enum_names<STATE>();
    for (auto &value : values) {
      fmt::print("{}\n", value);
    }
  }

  bool onExit() override { return true; }
};

class AbstractCommandLineMenu {
 private:
  std::vector<std::shared_ptr<CommandLineComponent>> components;
  std::map<std::string, std::shared_ptr<CommandLineComponent>> componentMap;
  std::shared_ptr<CommandLineComponent> current = nullptr;

  static std::map<std::string, std::shared_ptr<CommandLineComponent>> build(
      const std::vector<std::shared_ptr<CommandLineComponent>> &menuEntries) {
    std::map<std::string, std::shared_ptr<CommandLineComponent>> map;
    for (auto &menuEntry : menuEntries) {
      map.insert({menuEntry->getName(), menuEntry});
      map.insert({menuEntry->getAlias(), menuEntry});
    }
    return map;
  }

 public:
  explicit AbstractCommandLineMenu(
      std::vector<std::shared_ptr<CommandLineComponent>> &&components)
      : components(components), componentMap(build(components)) {}

  void onShowMenu() {
    if (current != nullptr) {
      current->onShowMenu();
      return;
    }
    fmt::print(
        fg(fmt::color::green) | fmt::emphasis::bold | fmt::emphasis::underline,
        "Menu Overview\n");
    int index = 0;
    for (auto &component : components) {
      fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}) {} [{}]\n",
                 ++index, component->getName(), component->getAlias());
    }
  }

  bool onMenu(const std::string &command) {
    if (current != nullptr) {
      return current->onMenu(command);
    }

    auto found = componentMap.find(command);
    if (found == componentMap.end()) {
      return false;
    }
    current = found->second;
    return true;
  }

  bool onExit() {
    bool success = current->onExit();
    if (current) {
      current = nullptr;
    }
    return success;
  }

  void onCommand(const std::string &command) {
    if (current == nullptr) {
      return;
    }
    current->onCommand(command);
  }

  void onHelp() {
    if (current == nullptr) {
      return;
    }
    current->onHelp();
  }

  std::shared_ptr<CommandLineComponent> currentOf() { return current; }
};

class CommandLineApplication {
 private:
  AbstractCommandLineMenu menu;
  std::thread thread;
  std::atomic<bool> running = true;

  enum COMMAND { COMMAND_HELP, COMMAND_MENU, COMMAND_EXIT };

  std::map<std::string, COMMAND> commands = {
      {"?", COMMAND_HELP},    {"help", COMMAND_HELP}, {"m", COMMAND_MENU},
      {"menu", COMMAND_MENU}, {"e", COMMAND_EXIT},    {"exit", COMMAND_EXIT}};

  void onComponentCommand(const std::string &command) {
    auto found = commands.find(command);
    if (found == commands.end()) {
      menu.onCommand(command);
      return;
    }
    switch (found->second) {
      case COMMAND_HELP:
        menu.onHelp();
        break;
      case COMMAND_MENU:
        menu.onShowMenu();
        break;
      case COMMAND_EXIT:
        menu.onExit();
        break;
      default:
        menu.onCommand(command);
        break;
    }
  }

  static void onStart() {
    std::string startInformation =
        "Welcome to the Command Line Interface:\n"
        "Try >help< or >?< for a list of commands\n"
        "Try >menue< or >m< to work with the menue system\n"
        "Try >exit< or >e< to go back or exit the Command Line Interface\n";
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, startInformation);
  }

  void onHelp() {
    // group commands map by command enum
    std::map<COMMAND, std::vector<std::string>> map;
    for (auto &iter : commands) {
      auto found = map.find(iter.second);
      if (found == map.end()) {
        map.insert({iter.second, {iter.first}});
      } else {
        found->second.push_back(iter.first);
      }
    }

    std::function<void(std::vector<std::string> &)> print =
        [&](std::vector<std::string> &aliases) {
          fmt::print(fg(fmt::color::green) | fmt::emphasis::italic, "[");
          for (std::size_t i = 0; i < aliases.size(); i++) {
            fmt::print(fg(fmt::color::green) | fmt::emphasis::italic, "{}",
                       aliases[i]);
            if (i < aliases.size() - 1) {
              fmt::print(fg(fmt::color::green) | fmt::emphasis::italic, ", ",
                         aliases[i]);
            }
          }
          fmt::print(fg(fmt::color::green) | fmt::emphasis::italic, "]\n");
        };

    fmt::print(
        fg(fmt::color::green) | fmt::emphasis::bold | fmt::emphasis::underline,
        "Help Overview\n");
    for (auto &[command, aliases] : map) {
      switch (command) {
        case COMMAND_MENU:
          fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                     "COMMAND_MENU: Shows available Menu entries");
          print(aliases);
          break;
        case COMMAND_EXIT:
          fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                     "COMMAND_EXIT: Exits current Menu or total CLI itself");
          print(aliases);
          break;
        case COMMAND_HELP:
          fmt::print(
              fg(fmt::color::green) | fmt::emphasis::bold,
              "COMMAND_HELP: Shows all available commands in current menu");
          print(aliases);
          break;
      }
    }
  }

  void onPrompt() {
    // timestamp MENU %
    std::string currentMenu =
        menu.currentOf() == nullptr ? "MAIN" : menu.currentOf()->getName();
    std::cout << currentMenu << " % ";
  }

  void start() {
    onStart();
    while (running) {
      onPrompt();
      std::string temp;
      std::getline(std::cin, temp);
      auto found = commands.find(temp);
      if (menu.currentOf() != nullptr) {
        onComponentCommand(temp);
        continue;
      }
      if (found == commands.end()) {
        bool success = menu.onMenu(temp);
        if (!success) {
          fmt::print(
              fg(fmt::color::red) | fmt::emphasis::bold,
              "ERROR: Invalid command '{}'! Please use the help function\n",
              temp);
        }
        continue;
      }
      switch (found->second) {
        case COMMAND_EXIT:
          SignalService::raiseSignal(SIGINT);
          running.store(false);
          break;
        case COMMAND_HELP:
          onHelp();
          break;
        case COMMAND_MENU:
          menu.onShowMenu();
          break;
      }
    }
  }

 public:
  explicit CommandLineApplication(
      std::vector<std::shared_ptr<CommandLineComponent>> &&components)
      : menu(std::move(components)), thread([&]() { start(); }) {}

  ~CommandLineApplication() {
    running.store(false);
    thread.join();
  }
};

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(argv[0]);
  std::shared_ptr<Component> websocketComponent =
      std::make_shared<WebsocketComponent>();
  std::vector<std::shared_ptr<Component>> components{websocketComponent};
  /* Get Websocket Configuration for Client */
  std::shared_ptr<Configuration> configuration =
      std::make_shared<Configuration>(components);
  // TODO replace hack with feature implementation of cli and provider for
  // getting correct Websocket Configuration
  std::shared_ptr<WebsocketEntry> websocketEntry =
      std::static_pointer_cast<WebsocketEntry>(
          configuration->configurationOf<WebsocketComponent>().at(1));
  std::shared_ptr<EchoController> echoController =
      std::make_shared<EchoController>();
  Client client(websocketEntry, {echoController});
  for (int i = 0; i < 220; i++) {
    std::shared_ptr<EchoRequest> request = std::make_shared<EchoRequest>();
    std::future<std::shared_ptr<Response>> response = client.send(request);
  }
  CommandLineApplication cli({std::make_shared<ConfigurationComponent>()});
  SignalService::registerHooks({SIGINT, SIGABRT, SIGTERM});
  SignalService::waitForUserInterrupt();
  return 0;
}