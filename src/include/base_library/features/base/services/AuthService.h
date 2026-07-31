#ifndef CPP_BASE_LIBRARY_AUTHSERVICE_H
#define CPP_BASE_LIBRARY_AUTHSERVICE_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/SchedulerService.h"

/**
 * Authentication service managing user login, access tokens, and session timeouts.
 */
class AuthService : public IAuthService,
                    public PropertyRegistration<AuthService>,
                    public std::enable_shared_from_this<AuthService> {
private:
    // properties
    std::shared_ptr<Property<std::chrono::seconds>> m_scheduleRate;
    std::shared_ptr<Property<std::chrono::minutes>> m_timeoutLogin;
    std::shared_ptr<Property<int32_t>> m_maxLoginFailures;
    std::shared_ptr<Property<std::chrono::seconds>> m_loginLockout;
    std::shared_ptr<Property<int32_t>> m_maxLoginAttempts;
    std::shared_ptr<Property<std::chrono::seconds>> m_rateLimitWindow;
    // variables
    std::map<std::string, UserToken> m_userTokens;
    std::mutex m_mutex;
    /** Failed login tracking per IP address for brute-force protection. */
    struct FailedAttempt {
        int32_t m_failures = 0;
        std::chrono::steady_clock::time_point m_firstFailure;
    };
    std::map<std::string, FailedAttempt> m_failedAttempts;
    /** Login attempt timestamps per IP for sliding-window rate limiting. */
    std::map<std::string, std::deque<std::chrono::steady_clock::time_point>>
            m_loginAttempts;
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<SchedulerService> m_scheduler;
    std::atomic_bool m_running = true;

    /** Periodically check for expired tokens. */
    void onCheck();

    /** @return true if the given IP is currently locked out (caller holds m_mutex). */
    bool isLockedOut(const std::string &ipAddress);

    /** Record a failed login attempt for the given IP (locks m_mutex). */
    void recordFailedLogin(const std::string &ipAddress);

    /** Clear failed login attempts for the given IP (locks m_mutex). */
    void clearFailedLogins(const std::string &ipAddress);

    /** @return true if a new login attempt is allowed for the IP (caller holds m_mutex). */
    bool allowLoginAttempt(const std::string &ipAddress);

    /** Remove expired login attempt entries (caller holds m_mutex). */
    void pruneLoginAttempts();

    /** Remove expired failed login entries (caller holds m_mutex). */
    void pruneFailedLogins();

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
    void onLogoutOf(const UserTokenLogin &userToken) override;

    /**
     * Get all active sessions of a user.
     * @param userName the username
     * @return list of active tokens of the user
     */
    std::vector<UserToken> allTokensOf(const std::string &userName) override;

    /**
     * Revoke a session by its token id.
     * @param id the token id to revoke
     */
    void revokeTokenOf(const std::string &id) override;

    void onInitialize() override;

    void onShutdown() override;
};

#endif  // CPP_BASE_LIBRARY_AUTHSERVICE_H
