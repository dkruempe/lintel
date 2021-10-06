#include "base_library/features/base/command_line/CryptionCliComponent.h"

#include <fmt/format.h>

#include <utility>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/controller/UserDto.h"

CryptionCliComponent::CryptionCliComponent(std::shared_ptr<UserApi> userApi)
    : CommandLineComponent(m_name, m_alias), m_userApi(std::move(userApi)) {}

void CryptionCliComponent::onCommand(
    const UserDto &userDto, const std::string &input,
    const std::vector<std::string> &parameters) {
  switch (m_currentCommand) {
    case CommandEncrypt:
      fmt::print("{}\n", m_cryption.encryption(input));
      m_currentCommand = CommandUndefined;
      return;
    case CommandDecrypt:
      fmt::print("{}\n", m_cryption.decryption(input));
      m_currentCommand = CommandUndefined;
      return;
    default:
      break;
  }
  auto found = m_commands.find(input);
  Command command =
      found == m_commands.end() ? CommandUndefined : found->second;
  switch (command) {
    case CommandEncrypt:
      fmt::print("Encryption of: \n");
      m_currentCommand = CommandEncrypt;
      break;
    case CommandDecrypt:
      fmt::print("Decryption of: \n");
      m_currentCommand = CommandDecrypt;
      break;
    case CommandUndefined:
      fmt::print("Wrong command \n");
      m_currentCommand = CommandUndefined;
      break;
  }
}
void CryptionCliComponent::onShowMenu() { fmt::print("No submenu available!"); }

bool CryptionCliComponent::onMenu(const std::string &component) { return true; }

void CryptionCliComponent::onHelp() {
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
      case CommandEncrypt:
        fmt::print("COMMAND_ENCRYPT: encrypts given string");
        print(aliases);
        break;
      case CommandDecrypt:
        fmt::print("COMMAND_DECRYPT: decrypt given strings");
        print(aliases);
        break;
      default:
        LOG_ERROR("undefined state");
        break;
    }
  }
}

bool CryptionCliComponent::onExit() { return true; }
void CryptionCliComponent::printCommandList() {}
