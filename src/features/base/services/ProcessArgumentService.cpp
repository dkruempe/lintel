#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/services/ProcessArgumentService.h"

#include <iomanip>
#include <sstream>

#include "base_library/core/services/LoggerService.h"

namespace {
void flushCurrentArgument(
        const std::string &flag, const std::vector<std::string> &args,
        Argument *temp) {
    if (temp == nullptr) {
        return;
    }
    if (args.size() > 1) {
        LOG_ERROR(
                "ignores other parameters bc. only 1 value per flag is currently "
                "allowed");
    }
    if (args.empty()) {
        temp->parse("");
        LOG_TRACE("parse flag {} with not argument", flag);
    } else {
        temp->parse(args[0]);
        LOG_TRACE("parse flag {} with argument {}", flag, args[0]);
    }
}
} // namespace

void ProcessArgumentService::processNewFlag(
        const std::string &argument, std::string &flag,
        std::vector<std::string> &args, Argument *&temp) {
    flag = argument;
    try {
        temp = &m_flagArgumentMap.at(flag);
    } catch (const std::exception &exception) {
        if (flag == "-h" || flag == "--help") {
            std::cout << "Arguments:\n";
            for (const auto &arg: m_arguments) {
                arg.printHelp();
            }
            std::cout << "\t --help, -h: list all arguments\n";
        } else {
            LOG_ERROR("flag {} not defined", flag);
            std::cerr << flag << ": not defined => ignore flag \n";
        }
        flag.clear();
        args.clear();
    }
}

void ProcessArgumentService::parseArguments(
        const std::vector<std::string> &arguments) {
    std::string flag;
    std::vector<std::string> args;
    Argument *temp = nullptr;
    for (const auto &argument: arguments) {
        if (argument.empty()) {
            continue;
        }
        LOG_TRACE("{}", argument);
        if (argument[0] == '-' && !flag.empty()) {
            flushCurrentArgument(flag, args, temp);
            flag.clear();
            args.clear();
        }
        if (argument[0] == '-') {
            processNewFlag(argument, flag, args, temp);
            continue;
        }
        args.push_back(argument);
    }
    if (!flag.empty()) {
        flushCurrentArgument(flag, args, temp);
    }
}

ProcessArgumentService::ProcessArgumentService(
        const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders)
        : m_flagArgumentMap(init(argumentProviders)),
          m_arguments(initArgs(argumentProviders)) {
    LOG_TRACE("count of argument providers {}", argumentProviders.size());
}

std::map<std::string, Argument> ProcessArgumentService::init(
        const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders) {
    std::map<std::string, Argument> map;
    for (auto &argumentProvider: argumentProviders) {
        for (const auto &argument: argumentProvider->provide()) {
            map.insert({argument.getFlag(), argument});
            map.insert({argument.getShortFlag(), argument});
        }
    }
    return map;
}

std::vector<Argument> ProcessArgumentService::initArgs(
        const std::vector<std::shared_ptr<ArgumentProvider>> &vector) {
    std::vector<Argument> args;
    for (const auto &item: vector) {
        const auto &vec = item->provide();
        std::transform(vec.begin(), vec.end(), std::back_inserter(args), [](const Argument &argument)  {
          return argument;
        });
    }
    return args;
}


