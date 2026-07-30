#ifndef CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
#define CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H

#include <map>
#include <memory>
#include <vector>

#include "base_library/core/utils/Argument.h"

/**
 * Service for parsing command-line arguments and populating a map of flags to Argument objects.
 */
class ProcessArgumentService {
private:
    std::map<std::string, Argument> m_flagArgumentMap;
    std::vector<Argument> m_arguments;

    static std::map<std::string, Argument> init(
            const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders);

    static std::vector<Argument> initArgs(
            const std::vector<std::shared_ptr<ArgumentProvider>> &vector);

public:
    /**
     * Constructor.
     * @param argumentProviders providers that register known flags
     */
    explicit ProcessArgumentService(
            const std::vector<std::shared_ptr<ArgumentProvider>> &argumentProviders);

    /**
     * Parse command-line arguments.
     * @param arguments argument strings to parse
     */
    void parseArguments(const std::vector<std::string> &arguments);

    /**
     * Process a new flag and its associated arguments.
     * @param argument current argument string
     * @param flag current flag being processed
     * @param args collected arguments for the flag
     * @param temp pointer to the current Argument being populated
     */
    void processNewFlag(const std::string &argument, std::string &flag,
                        std::vector<std::string> &args, Argument *&temp);
};

#endif  // CPP_BASE_LIBRARY_PROCESSARGUMENTSERVICE_H
