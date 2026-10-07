#ifndef LINTEL_COMMAND_H
#define LINTEL_COMMAND_H

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/** Represents a CLI command with named arguments and automatic parsing */
class Command {
public:
    // These are the possible variables the options may point to. Bool and
    // std::string are handled in a special way, all other values are parsed
    // with a std::stringstream. This std::variant can be easily extended if
    // the stream operator>> is overloaded. If not, you have to add a special
    // case to the parse() method.
    using Value =
            std::variant<int32_t *, uint32_t *, double *, float *, bool *,
                    std::string *, std::vector<std::string> *,
                    std::optional<int32_t> *, std::optional<uint32_t> *,
                    std::optional<double> *, std::optional<float> *,
                    std::optional<bool> *, std::optional<std::string> *>;

    /** @param command the command name
     *  @param description description printed in help */
    Command(std::string command, std::string description);

    /**
     * Adds a possible option. A typical call would be like this:
     * bool printHelp = false;
     * cmd.addArgument({"--help", "-h"}, &printHelp, "Print this help message");
     * Then, after parse() has been called, printHelp will be true if the user
     * provided the flag.
     * @param flags list of flag aliases (e.g. {"--help", "-h"})
     * @param defaultValue pointer to the target variable
     * @param help help text for this argument
     * @param optional whether the argument is optional
     */
    Command &addArgument(const std::vector<std::string> &flags,
                         Value defaultValue, const std::string &help,
                         bool optional = false);

    /** @overload */
    Command &addArgument(const std::vector<std::string> &&flags,
                         Value defaultValue, std::string help,
                         bool optional = false);

    /** Print the command description and help for each option */
    void printHelp(std::ostream &os = std::cout) const;

    /** @return the command name */
    [[nodiscard]] std::string getCommand() const;

    /**
     * Parse command-line arguments. Arguments are traversed from start to end;
     * if an option is set multiple times, the last one wins.
     * @param command the command string
     * @param flags the flag values to parse
     * @throws std::runtime_error if a value is missing for a given option
     */
    void parse(const std::string &command,
               const std::vector<std::string> &flags) const;

    /** Reset all optional values to their defaults */
    void resetOptionals() const;

    /** Parse a single value from string into the variant target */
    static void parseValueFromStream(const std::string &value,
                                     const Value &m_value);

private:
    struct Argument {
        std::vector<std::string> m_flags;
        Value m_value;
        std::string m_help;
        bool m_optional;
    };

    std::string m_command;
    std::string m_description;
    std::map<std::string, Argument> m_argumentsMap;
    std::vector<Argument> m_arguments;
    static constexpr const char *m_tab = "\t";
};

#endif  // LINTEL_COMMAND_H
