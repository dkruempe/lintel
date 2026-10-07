#ifndef LINTEL_ARGUMENT_H
#define LINTEL_ARGUMENT_H

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/** Represents a single command-line argument with flag, short flag, description, and parsed value. */
class Argument {
private:
    std::string m_flag;
    std::string m_shortFlag;
    std::string m_description;

public:
    typedef std::variant<int32_t *, uint32_t *, double *, float *, bool *,
            std::string *, std::optional<int32_t> *,
            std::optional<uint32_t> *, std::optional<double> *,
            std::optional<float> *, std::optional<bool> *,
            std::optional<std::string> *>
            Value;

    /** Construct an Argument with its flags, description, and target value.
     * @param flag        the long flag name (e.g. "--verbose")
     * @param shortFlag   the short flag name (e.g. "-v")
     * @param description the help description
     * @param value       pointer to the variable to store the parsed value */
    Argument(std::string flag, std::string shortFlag, std::string description,
             const Value &value);

    /** Print the help text for this argument to the given stream.
     * @param os the output stream (default: std::cout) */
    void printHelp(std::ostream &os = std::cout) const;

    /** Parse a command-line argument string and store the result.
     * @param arguments the argument string to parse */
    void parse(const std::string &arguments);

    /** Returns the long flag name.
     * @return the long flag */
    [[nodiscard]] const std::string &getFlag() const;

    /** Returns the short flag name.
     * @return the short flag */
    [[nodiscard]] const std::string &getShortFlag() const;

private:
    void resetOptionals();

    void parseOptionalFromStream(const std::string &argument);

    Value m_value;
};

/** Provider of a collection of command-line Argument definitions. */
class ArgumentProvider {
private:
    std::vector<Argument> m_arguments;

public:
    /** Construct an ArgumentProvider with a list of argument definitions.
     * @param arguments the argument definitions */
    explicit ArgumentProvider(std::vector<Argument> arguments);

    /** Returns the list of argument definitions.
     * @return the argument vector */
    [[nodiscard]] const std::vector<Argument> &provide() const;
};

#endif  // LINTEL_ARGUMENT_H
