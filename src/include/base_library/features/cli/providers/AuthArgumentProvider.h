#ifndef CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H
#define CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H

#include "base_library/core/utils/Argument.h"

/** Provides authentication-related command-line arguments (e.g. --username) */
class AuthArgumentProvider : public ArgumentProvider {
private:
    // -u --username
    std::optional<std::string> m_userName;

public:
    AuthArgumentProvider();

    /** Initialize and return the supported arguments */
    std::vector<Argument> init();

    /** @return the parsed username, if provided */
    std::optional<std::string> getUserName();
};

#endif  // CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H
