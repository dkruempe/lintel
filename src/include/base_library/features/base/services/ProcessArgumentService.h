#ifndef CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
#define CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H

#include <map>
#include <memory>
#include <vector>

#include "base_library/core/utils/Argument.h"

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

    void processNewFlag(const std::string &argument, std::string &flag,
                        std::vector<std::string> &args, Argument *&temp);
};

#endif  // CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
