#include "base_library/features/cli/models/Command.h"

#include <iomanip>
#include <sstream>

#include "base_library/core/utils/StringUtils.h"

Command::Command(std::string command, std::string description)
        : m_command(std::move(command)), m_description(std::move(description)) {}

Command &Command::addArgument(const std::vector<std::string> &flags,
                              Command::Value value, const std::string &help,
                              bool optional) {
    Argument argument{flags, value, help, optional};
    for (const auto &flag: flags) {
        m_argumentsMap.insert({flag, argument});
    }
    m_arguments.push_back(argument);
    return *this;
}

Command &Command::addArgument(const std::vector<std::string> &&flags,
                              Value defaultValue, std::string help,
                              bool optional) {
    Argument argument{flags, defaultValue, std::move(help), optional};
    for (const auto &flag: flags) {
        m_argumentsMap.insert({flag, argument});
    }
    m_arguments.push_back(argument);
    return *this;
}

void Command::printHelp(std::ostream &os) const {
    // Print the general description.
    os << m_command << ":\n";
    os << m_tab << m_description << "\n";
    if (!m_arguments.empty()) {
        os << m_tab << "\n";
        os << m_tab << "Arguments: \n";
    }

    // Find the argument with the longest combined flag length (in order
    // to align the help messages).

    uint32_t maxFlagLength = 0;

    for (auto const &argument: m_arguments) {
        uint32_t flagLength = 0;
        for (auto const &flag: argument.m_flags) {
            // Plus comma and space.
            flagLength += static_cast<uint32_t>(flag.size()) + 2;
        }

        maxFlagLength = std::max(maxFlagLength, flagLength);
    }

    // Now print each argument.
    for (auto const &argument: m_arguments) {
        std::string flags;
        for (auto const &flag: argument.m_flags) {
            flags += flag + ", ";
        }

        // Remove last comma and space and add padding according to the
        // longest flags in order to align the help messages.
        std::stringstream sstr;
        sstr << std::left << std::setw(static_cast<int>(maxFlagLength))
             << flags.substr(0, flags.size() - 2);

        // Print the help for each argument. This is a bit more involved
        // since we do line wrapping for long descriptions.
        size_t spacePos = 0;
        size_t lineWidth = 0;
        while (spacePos != std::string::npos) {
            size_t nextspacePos = argument.m_help.find_first_of(' ', spacePos + 1);
            sstr << argument.m_help.substr(spacePos, nextspacePos - spacePos);
            lineWidth += nextspacePos - spacePos;
            spacePos = nextspacePos;

            if (lineWidth > 60) {
                os << m_tab << sstr.str() << std::endl;
                sstr = std::stringstream();
                sstr << std::left << std::setw(static_cast<int>(maxFlagLength - 1))
                     << " ";
                lineWidth = 0;
            }
        }
    }
}

void Command::parse(const std::string &command,
                    const std::vector<std::string> &flags) const {
    for (auto &argument: m_arguments) {
        if (std::holds_alternative<std::optional<std::string> *>(argument.m_value)) {
            *std::get<std::optional<std::string> *>(argument.m_value) = std::nullopt;
        } else if (std::holds_alternative<std::optional<bool> *>(argument.m_value)) {
            *std::get<std::optional<bool> *>(argument.m_value) = std::nullopt;
        } else if (std::holds_alternative<std::optional<float> *>(
                argument.m_value)) {
            *std::get<std::optional<float> *>(argument.m_value) = std::nullopt;
        } else if (std::holds_alternative<std::optional<double> *>(
                argument.m_value)) {
            *std::get<std::optional<double> *>(argument.m_value) = std::nullopt;
        } else if (std::holds_alternative<std::optional<uint32_t> *>(
                argument.m_value)) {
            *std::get<std::optional<uint32_t> *>(argument.m_value) = std::nullopt;
        } else if (std::holds_alternative<std::optional<int32_t> *>(
                argument.m_value)) {
            *std::get<std::optional<int32_t> *>(argument.m_value) = std::nullopt;
        }
    }
    std::vector<Argument> argumentsVec;
    // Skip the first argument (name of the program).
    for (std::size_t i = 0; i < flags.size(); i++) {
        // First we have to identify wether the value is separated by a space
        // or a '='.
        std::string flag(flags[i]);
        std::string value;
        bool valueIsSeparate = false;

        // If there is an '=' in the flag, the part after the '=' is actually
        // the value.
        size_t equalPos = flag.find('=');
        if (equalPos != std::string::npos) {
            value = flag.substr(equalPos + 1);
            flag = flag.substr(0, equalPos);
        }
            // Else the following argument is the value.
        else if (i + 1 < flags.size()) {
            value = flags[i + 1];
            valueIsSeparate = true;
        }

        auto found = m_argumentsMap.find(flag);
        if (found == m_argumentsMap.end()) {
            std::cerr << "Ignoring unknown command line argument \"" << flag << "\"."
                      << std::endl;
            continue;
        }
        const Argument &argument = found->second;
        // In the case of booleans, there must not be a value present.
        // So if the value is neither 'true' nor 'false' it is considered
        // to be the next argument.
        if (std::holds_alternative<bool *>(argument.m_value)) {
            if (!value.empty() && value != "true" && value != "false") {
                valueIsSeparate = false;
            }
            *std::get<bool *>(argument.m_value) = (value != "false");
        } else if (std::holds_alternative<std::optional<bool> *>(argument.m_value)) {
            if (!value.empty() && value != "true" && value != "false") {
                valueIsSeparate = false;
            }
            *std::get<std::optional<bool> *>(argument.m_value) =
                    std::make_optional(value != "false");
        }
            // In all other cases there must be a value.
        else if (value.empty()) {
            throw std::runtime_error(
                    "Failed to parse command line arguments: "
                    "Missing value for argument \"" +
                    flag + "\"!");
        }
            // For a std::string, we take the entire value.
        else if (std::holds_alternative<std::string *>(argument.m_value)) {
            *std::get<std::string *>(argument.m_value) = value;
        } else if (std::holds_alternative<std::optional<std::string> *>(
                argument.m_value)) {
            *std::get<std::optional<std::string> *>(argument.m_value) =
                    std::make_optional(value);
        } else {
            std::visit(
                    [&value](auto &&arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same<T, std::optional<std::string> *>::value) {
                            std::string temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<T, std::optional<bool> *>::value) {
                            bool temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<T,
                                std::optional<double> *>::value) {
                            double temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<T,
                                std::optional<float> *>::value) {
                            double temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<
                                T, std::optional<uint32_t> *>::value) {
                            uint32_t temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<T,
                                std::optional<int32_t> *>::value) {
                            int32_t temp;
                            std::stringstream sstr(value);
                            sstr >> temp;
                            *arg = std::make_optional(temp);
                        } else if constexpr (std::is_same<
                                T, std::vector<std::string> *>::value) {
                            std::vector<std::string> temp;
                            temp = StringUtils::split(value, ',');
                            *arg = temp;
                        } else {
                            std::stringstream sstr(value);
                            sstr >> *arg;
                        }
                    },
                    argument.m_value);
        }

        argumentsVec.push_back(argument);

        // If the value was separated, we have to advance our index once more.
        if (valueIsSeparate) {
            ++i;
        }
    }
}

std::string Command::getCommand() const { return m_command; }
