#ifndef CPP_BASE_LIBRARY_AUTHSERVICE_H
#define CPP_BASE_LIBRARY_AUTHSERVICE_H

#include <atomic>
#include <map>

#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/SchedulerService.h"

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
