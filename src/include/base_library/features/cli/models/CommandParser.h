#ifndef CPP_BASE_LIBRARY_COMMANDPARSER_H
#define CPP_BASE_LIBRARY_COMMANDPARSER_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <optional>
#include <string>
#include <vector>

#include "base_library/features/cli/models/Command.h"

/**
 * Generic command parser that maps string commands to enum values
 * @tparam ENUM enum type for command identifiers
 * @tparam undefined enum value returned for unknown commands
 */
template<typename ENUM, ENUM undefined>
class CommandParser {
private:
    std::map<std::string, Command> m_commands;
    std::map<std::string, ENUM> m_enumCommand;
    static constexpr int32_t m_maxCommandPerLine = 10;

public:
    CommandParser() = default;

    /** Register a command with its enum value */
    void addCommand(const Command &command, ENUM value) {
        m_commands.insert({command.getCommand(), command});
        m_enumCommand.insert({command.getCommand(), value});
    }

    /** @overload */
    void addCommand(Command &&command, ENUM value) {
        m_commands.insert({command.getCommand(), command});
        m_enumCommand.insert({command.getCommand(), value});
    }

    /**
     * Parse a command string and its flags
     * @param command the command name
     * @param flags the flag arguments
     * @return the matching enum value, or undefined if not found
     */
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

    /** Print help for a specific command */
    void printHelp(const std::string &command, std::ostream &os = std::cout) {
        auto found = m_commands.find(command);
        if (found == m_commands.end()) {
            os << "ERROR: " << command << " not available\n";
            return;
        }
        found->second.printHelp(os);
        os << "\n";
    }

    /** Print full help page for all commands in this component */
    void printHelp(std::string_view componentName, const std::string_view alias,
                   std::string_view description, std::ostream &os = std::cout) {
        os << "Help Menu of " << componentName << " [" << alias << "]:\n\n";
        os << description << "\n\n";
        for (const auto &item: m_commands) {
            item.second.printHelp(os);
            os << "\n";
        }
    }

    /** Print a compact list of all commands and aliases */
    void printCommandList(std::set<std::string> menuAlias, std::ostream &os = std::cout) {
        os << "\n\t";
        int32_t i = 0;
        for (const auto &item: m_commands) {
            i++;
            if (i % m_maxCommandPerLine == 0) {
                os << "\n";
            }
            os << item.first;
            if (i % m_maxCommandPerLine > 0) {
                os << "\t";
            }
        }
        for (const auto &item: menuAlias) {
            i++;
            if (i % m_maxCommandPerLine == 0) {
                os << "\n";
            }
            os << item;
            if (i % m_maxCommandPerLine > 0) {
                os << "\t";
            }
        }
        os << "\n";
    }

    /** @return vector of all registered command name strings */
    std::vector<std::string> allCommandsOf() {
        std::vector<std::string> temp;
        std::transform(
                m_commands.begin(), m_commands.end(), std::back_inserter(temp),
                [](const std::pair<std::string, Command> &pair) -> std::string {
                    return pair.second.getCommand();
                });
        return temp;
    }
};

#endif  // CPP_BASE_LIBRARY_COMMANDPARSER_H
