#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/cli/providers/AuthArgumentProvider.h"

#include "base_library/core/services/LoggerService.h"

AuthArgumentProvider::AuthArgumentProvider() : ArgumentProvider(init()) {
    LOG_TRACE("call constructor");
}

std::vector<Argument> AuthArgumentProvider::init() {
    std::vector<Argument> arguments;
    arguments.emplace_back("--username", "-u", "username for cli login",
                           &m_userName);
    return arguments;
}

std::optional<std::string> AuthArgumentProvider::getUserName() {
    if (m_userName.has_value()) {
        LOG_TRACE("{}", m_userName.value());
    } else {
        LOG_TRACE("user_name has no value");
    }
    return m_userName;
}
