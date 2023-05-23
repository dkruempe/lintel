#ifndef CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
#define CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base_library/core/utils/StringUtils.h"

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

    Argument(std::string flag, std::string shortFlag, std::string description,
             const Value &value);

    void printHelp(std::ostream &os = std::cout) const;

    void parse(const std::string &arguments);

    [[nodiscard]] const std::string &getFlag() const;

    [[nodiscard]] const std::string &getShortFlag() const;

private:
    Value m_value;
};

class ArgumentProvider {
private:
    std::vector<Argument> m_arguments;

public:
    explicit ArgumentProvider(std::vector<Argument> arguments);

    [[nodiscard]] const std::vector<Argument> &provide() const;
};

class ProcessArgumentService {
private:
    std::map<std::string, Argument> m_flagArgumentMap;
    std::vector<Argument> m_arguments;

    static std::map<std::string, Argument> init(
            const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders);

    static std::vector<Argument> initArgs(
            const std::vector<std::shared_ptr<ArgumentProvider>> &vector);

public:
    explicit ProcessArgumentService(
            const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders);

    void parseArguments(const std::vector<std::string> &arguments);
};

#endif  // CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
