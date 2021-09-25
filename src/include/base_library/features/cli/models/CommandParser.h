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
  static constexpr int32_t m_maxCommandPerLine = 10;

 public:
  CommandParser() = default;

  void addCommand(const Command &command, ENUM value) {
    m_commands.insert({command.getCommand(), command});
    m_enumCommand.insert({command.getCommand(), value});
  }

  void addCommand(Command &&command, ENUM value) {
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

  void printHelp(const std::string &command, std::ostream &os = std::cout) {
    auto found = m_commands.find(command);
    if (found == m_commands.end()) {
      os << "ERROR: " << command << " not available\n";
      return;
    }
    found->second.printHelp(os);
    os << "\n";
  }

  void printHelp(std::string_view componentName, const std::string_view alias,
                 std::string_view description, std::ostream &os = std::cout) {
    os << "Help Menu of " << componentName << " [" << alias << "]:\n\n";
    os << description << "\n\n";
    for (const auto &item : m_commands) {
      item.second.printHelp(os);
      // make break between all kind of commands for an easier separation
      os << "\n";
    }
  }

  void printCommandList(std::ostream &os = std::cout) {
    os << "\n\t";
    int32_t i = 0;
    for (const auto &item : m_commands) {
      i++;
      if (i % m_maxCommandPerLine == 0) {
        os << "\n";
      }
      os << item.first;
      if (i % m_maxCommandPerLine > 0) {
        os << "\t";
      }
    }
    os << "\n";
  }
};

#endif  // CPP_BASE_LIBRARY_COMMANDPARSER_H
