#ifndef CPP_BASE_LIBRARY_AUTHSERVICE_H
#define CPP_BASE_LIBRARY_AUTHSERVICE_H

#include <atomic>
#include <map>

#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/SchedulerService.h"

/**
 * Authentication service managing user login, access tokens, and session timeouts.
 */
class AuthService : public IAuthService, public PropertyRegistration<AuthService> {
private:
    // properties
    std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
    std::shared_ptr<Property<std::chrono::minutes>> m_timeoutLogin;
    // variables
    std::map<std::string, UserToken> m_userTokens;
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<SchedulerService> m_scheduler;
    std::atomic_bool m_running = true;

    /** Periodically check for expired tokens. */
    void onCheck();

public:
    /**
     * Constructor.
     * @param processName current process name
     * @param userRepository user repository
     * @param schedulerService scheduler for periodic token checks
     */
    AuthService(const std::shared_ptr<ProcessName> &processName,
                std::shared_ptr<UserRepository> userRepository,
                std::shared_ptr<SchedulerService> schedulerService);

    virtual ~AuthService() = default;

    /**
     * Authenticate a user with username and password.
     * @param userLogin login credentials
     * @return a user token if authentication succeeds
     */
    std::optional<UserToken> onLoginOf(const UserLogin &userLogin);

    /**
     * Validate an existing user token for access.
     * @param userTokenLogin token to validate
     * @return the user token if valid
     */
    std::optional<UserToken> onAccessOf(const UserTokenLogin &userTokenLogin);

    /**
     * Log out and invalidate a user token.
     * @param userToken token to invalidate
     */
    void onLogoutOf(const UserTokenLogin &userToken);

    void onInitialize() override;

    void onShutdown() override;
};

#endif  // CPP_BASE_LIBRARY_AUTHSERVICE_H
