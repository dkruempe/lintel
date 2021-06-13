#include "base_library/features/base/command_line/CryptionCommandLineComponent.h"

#include <fmt/color.h>
#include <fmt/core.h>

#include <future>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/controller/EchoRequest.h"
#include "base_library/features/websocket/controller/EchoResponse.h"

CryptionComamndLineComponent::CryptionComamndLineComponent()
    : CommandLineComponent(m_name, m_alias) {}

void CryptionComamndLineComponent::onCommand(const std::string &input) {
  switch (m_currentCommand) {
    case COMMAND_ENCRYPT:
      fmt::print("{}\n", m_cryption.encryption(input));
      m_currentCommand = COMMAND_UNDEFINED;
      return;
    case COMMAND_DECRYPT:
      fmt::print("{}\n", m_cryption.decryption(input));
      m_currentCommand = COMMAND_UNDEFINED;
      return;
    default:
      break;
  }
  auto found = m_commands.find(input);
  COMMAND command =
      found == m_commands.end() ? COMMAND_UNDEFINED : found->second;
  switch (command) {
    case COMMAND_ENCRYPT:
      fmt::print("Encryption of: \n");
      m_currentCommand = COMMAND_ENCRYPT;
      break;
    case COMMAND_DECRYPT:
      fmt::print("Decryption of: \n");
      m_currentCommand = COMMAND_DECRYPT;
      break;
    case COMMAND_MESSAGE:
      for (int i = 0; i < 220; i++) {
        std::shared_ptr<EchoRequest> request = std::make_shared<EchoRequest>();
        std::future<std::shared_ptr<Response>> response =
            getClient()->send(request);
      }
      fmt::print("Send messages to websocket Service \n");
      m_currentCommand = COMMAND_UNDEFINED;
      break;
    case COMMAND_UNDEFINED:
      fmt::print("Wrong command \n");
      m_currentCommand = COMMAND_UNDEFINED;
      break;
  }
}
void CryptionComamndLineComponent::onShowMenu() {
  fmt::print("No submenu available!");
}

bool CryptionComamndLineComponent::onMenu(const std::string &component) {
  return true;
}

void CryptionComamndLineComponent::onHelp() {
  // group commands map by command enum
  std::map<COMMAND, std::vector<std::string_view>> map;
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
      case COMMAND_ENCRYPT:
        fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                   "COMMAND_ENCRYPT: encrypts given string");
        print(aliases);
        break;
      case COMMAND_DECRYPT:
        fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                   "COMMAND_DECRYPT: decrypt given strings");
        print(aliases);
        break;
      case COMMAND_MESSAGE:
        fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                   "COMMAND_MESSAGE: send test messages to websocket");
        print(aliases);
        break;
        break;
      default:
        LOG_ERROR("undefined state");
        break;
    }
  }
}

bool CryptionComamndLineComponent::onExit() { return true; }
