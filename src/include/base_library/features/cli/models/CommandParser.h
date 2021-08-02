#ifndef CPP_BASE_LIBRARY_COMMANDPARSER_H
#define CPP_BASE_LIBRARY_COMMANDPARSER_H

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base_library/features/cli/models/Command.h"

template <typename ENUM, ENUM undefined>
class CommandParser {
 private:
  /*
   * List of valid commands with needed parameters
   * std::map<Command, Parameters>
   */
  std::map<std::string, Command> m_commands;
  std::map<std::string, ENUM> m_enumCommand;

 public:
  CommandParser() = default;

  void addCommand(const Command &command, ENUM value) {
    m_commands.insert({command.getCommand(), command});
    m_enumCommand.insert({command.getCommand(), value});
  }

  ENUM parse(const std::string &command,
             const std::vector<std::string> &flags) {
    auto foundEnum = m_enumCommand.find(command);
    auto found = m_commands.find(command);
    if (foundEnum == m_enumCommand.end() || found == m_commands.end()) {
      return undefined;
    }
    found->second.parse(command, flags);
    return foundEnum->second;
  }

  void printHelp(std::ostream &os = std::cout) {
    for (const auto &item : m_commands) {
      item.second.printHelp(os);
    }
  }
};

#endif  // CPP_BASE_LIBRARY_COMMANDPARSER_H
