#ifndef CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H
#define CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H

#include "base_library/core/utils/Argument.h"

class AuthArgumentProvider : public ArgumentProvider {
private:
    // -u --username
    std::optional<std::string> m_userName;

public:
    AuthArgumentProvider();

    std::vector<Argument> init();

    std::optional<std::string> getUserName();
};

#endif  // CPP_BASE_LIBRARY_AUTHARGUMENTPROVIDER_H
