#ifndef CPP_BASE_LIBRARY_AUTHSERVICE_H
#define CPP_BASE_LIBRARY_AUTHSERVICE_H

#include <atomic>
#include <map>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/SchedulerService.h"

class AuthService : public IAuthService, public AbstractService<AuthService> {
private:
    // properties
    DEFINE_PROPERTY(m_scheduleRate, std::chrono::seconds, std::chrono::seconds(2),
                    "schedule rate of user tokens checks in seconds", true);
    DEFINE_PROPERTY(m_timeoutLogin, std::chrono::minutes, std::chrono::minutes(5),
                    "timeout of user login in minutes", true);
    // variables
    std::map<std::string, UserToken> m_userTokens;
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<SchedulerService> m_scheduler;
    std::atomic_bool m_running = true;

    void onCheck();

public:
    AuthService(const std::shared_ptr<ProcessName> &processName,
                std::shared_ptr<UserRepository> userRepository,
                std::shared_ptr<SchedulerService> schedulerService);

    virtual ~AuthService() = default;

    std::optional<UserToken> onLoginOf(const UserLogin &userLogin);

    std::optional<UserToken> onAccessOf(const UserTokenLogin &userTokenLogin);

    void onLogoutOf(const UserTokenLogin &userToken);

    void onInitialize() override;

    void onShutdown() override;
};

#endif  // CPP_BASE_LIBRARY_AUTHSERVICE_H
