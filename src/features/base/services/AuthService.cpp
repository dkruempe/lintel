#include "base_library/features/base/services/AuthService.h"

#include <date/date.h>

#include <boost/asio.hpp>
#include <chrono>
#include <vector>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/Cryption.h"
#include "base_library/core/utils/UUID.h"

AuthService::AuthService(const std::shared_ptr<ProcessName> &processName,
                         std::shared_ptr<UserRepository> userRepository,
                         std::shared_ptr<SchedulerService> schedulerService)
        : PropertyRegistration(processName->getProcessName()),
          m_userRepository(std::move(userRepository)),
          m_scheduler(std::move(schedulerService)) {
    m_scheduleRate = registerProperty<std::chrono::seconds>(
            "m_scheduleRate", std::chrono::seconds(2),
            "schedule rate of user tokens checks in seconds", true,
            __FILE__, __LINE__);
    m_timeoutLogin = registerProperty<std::chrono::minutes>(
            "m_timeoutLogin", std::chrono::minutes(5),
            "timeout of user login in minutes", true,
            __FILE__, __LINE__);
    m_maxLoginFailures = registerProperty<int32_t>(
            "m_maxLoginFailures", 5,
            "maximum failed login attempts per IP before lockout", true,
            __FILE__, __LINE__);
    m_loginLockout = registerProperty<std::chrono::seconds>(
            "m_loginLockout", std::chrono::seconds(60),
            "lockout duration after too many failed login attempts", true,
            __FILE__, __LINE__);
    m_maxLoginAttempts = registerProperty<int32_t>(
            "m_maxLoginAttempts", 20,
            "maximum login attempts per IP within the rate limit window", true,
            __FILE__, __LINE__);
    m_rateLimitWindow = registerProperty<std::chrono::seconds>(
            "m_rateLimitWindow", std::chrono::seconds(60),
            "sliding window for login attempt rate limiting in seconds", true,
            __FILE__, __LINE__);
}

std::optional<UserToken> AuthService::onLoginOf(const UserLogin &userLogin) {
    // I first easy check of ipaddress
    if (userLogin.m_ipAddress.empty()) {
        LOG_ERROR("ip address of {} empty", userLogin.m_userName);
        return std::nullopt;
    }
    // II check via boost ip address
    boost::system::error_code ec{};
    auto ip = boost::asio::ip::make_address(userLogin.m_ipAddress, ec);
    if (ip.to_string() != userLogin.m_ipAddress) {
        LOG_ERROR("{} invalid ip address of {}", userLogin.m_ipAddress,
                  userLogin.m_userName);
        return std::nullopt;
    }
    // IIb brute-force protection: reject locked-out IPs
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (isLockedOut(userLogin.m_ipAddress)) {
            LOG_WARN("{}: login attempts for {} blocked (lockout)",
                     userLogin.m_ipAddress, userLogin.m_userName);
            return std::nullopt;
        }
    }
    // IIc sliding-window rate limiting per IP
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!allowLoginAttempt(userLogin.m_ipAddress)) {
            LOG_WARN("{}: login attempts for {} rate limited",
                     userLogin.m_ipAddress, userLogin.m_userName);
            return std::nullopt;
        }
    }
    // III check if user is logged in
    auto userOpt = m_userRepository->of(userLogin.m_userName);
    if (!userOpt.has_value()) {
        LOG_INFO("{}: {} does not exists", userLogin.m_ipAddress,
                 userLogin.m_userName);
        recordFailedLogin(userLogin.m_ipAddress);
        return std::nullopt;
    }
    const User &user = userOpt.value();
    if (!Cryption::verifyOf(userLogin.m_password, user.getPassword())) {
        LOG_INFO("{}: {} password invalid", userLogin.m_ipAddress,
                 userLogin.m_userName);
        recordFailedLogin(userLogin.m_ipAddress);
        return std::nullopt;
    }
    clearFailedLogins(userLogin.m_ipAddress);
    // IIIb transparent upgrade of legacy (unsalted SHA-512) hashes
    if (!Cryption::isModernHash(user.getPassword())) {
        try {
            m_userRepository->changePasswordOf(
                    user, Cryption::hashOf(userLogin.m_password));
            LOG_INFO("{}: {} password hash upgraded to salted format",
                     userLogin.m_ipAddress, userLogin.m_userName);
        } catch (const std::exception &exception) {
            LOG_ERROR("{}: cannot upgrade password hash of {}: {}",
                      userLogin.m_ipAddress, userLogin.m_userName,
                      exception.what());
        }
    }
    std::string id = UUID::generate();
    UserToken userToken{userLogin.m_ipAddress, id,
                        std::chrono::time_point_cast<std::chrono::microseconds>(
                                std::chrono::system_clock::now()),
                        user};
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_userTokens.insert({userToken.m_id, userToken});
    }
    LOG_INFO("{}: {} {} success full login", userLogin.m_ipAddress,
             userLogin.m_userName, id);
    return userToken;
}

std::optional<UserToken> AuthService::onAccessOf(
        const UserTokenLogin &userTokenLogin) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto found = m_userTokens.find(userTokenLogin.m_id);
    if (found == m_userTokens.end()) {
        LOG_ERROR("{}: {} user not logged in", userTokenLogin.m_ipAddress,
                  userTokenLogin.m_id);
        return std::nullopt;
    }
    if (userTokenLogin.m_ipAddress.empty() ||
        found->second.m_ipAddress != userTokenLogin.m_ipAddress) {
        LOG_WARN("{}: {} ip address mismatch (expected {})",
                 userTokenLogin.m_ipAddress, userTokenLogin.m_id,
                 found->second.m_ipAddress);
        return std::nullopt;
    }
    found->second.m_lastAccessTimestamps =
            std::chrono::time_point_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now());
    return std::make_optional(found->second);
}

void AuthService::onLogoutOf(const UserTokenLogin &userToken) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto found = m_userTokens.find(userToken.m_id);
    if (found == m_userTokens.end()) {
        LOG_ERROR("{}: {} user not logged in", userToken.m_ipAddress,
                  userToken.m_id);
        throw std::runtime_error("user not logged in");
    }
    if (found->second.m_ipAddress != userToken.m_ipAddress) {
        LOG_WARN("{}: {} logout blocked, ip address mismatch (expected {})",
                 userToken.m_ipAddress, userToken.m_id,
                 found->second.m_ipAddress);
        return;
    }
    m_userTokens.erase(userToken.m_id);
}

std::vector<UserToken> AuthService::allTokensOf(const std::string &userName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<UserToken> result;
    for (const auto &iter: m_userTokens) {
        if (iter.second.m_user.getUserName() == userName) {
            result.push_back(iter.second);
        }
    }
    return result;
}

void AuthService::revokeTokenOf(const std::string &id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_userTokens.erase(id);
}

void AuthService::onCheck() {
    date::sys_time<std::chrono::microseconds> now =
            std::chrono::time_point_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now());
    std::vector<std::string> toBeRemoved;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &iter: m_userTokens) {
            if ((now - iter.second.m_lastAccessTimestamps) <
                m_timeoutLogin->getValue()) {
                continue;
            }
            LOG_INFO("{}:{} timeout login", iter.second.m_ipAddress, iter.second.m_id);
            toBeRemoved.push_back(iter.first);
        }
        for (const auto &iter: toBeRemoved) {
            m_userTokens.erase(iter);
        }
        pruneFailedLogins();
        pruneLoginAttempts();
    }
    if (m_running) {
        std::weak_ptr<AuthService> weakSelf = shared_from_this();
        m_scheduler->schedule_after(m_scheduleRate->getValue(),
                                    [weakSelf]() {
                                        if (auto self = weakSelf.lock()) {
                                            self->onCheck();
                                        }
                                    });
    }
}

void AuthService::onInitialize() {
    std::weak_ptr<AuthService> weakSelf = shared_from_this();
    m_scheduler->schedule_after(m_scheduleRate->getValue(),
                                [weakSelf]() {
                                    if (auto self = weakSelf.lock()) {
                                        self->onCheck();
                                    }
                                });
}

void AuthService::onShutdown() {
    LOG_INFO("stop auth service");
    m_running.store(false);
}

bool AuthService::isLockedOut(const std::string &ipAddress) {
    auto found = m_failedAttempts.find(ipAddress);
    if (found == m_failedAttempts.end()) {
        return false;
    }
    const FailedAttempt &attempt = found->second;
    if (attempt.m_failures < m_maxLoginFailures->getValue()) {
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    return (now - attempt.m_firstFailure) < m_loginLockout->getValue();
}

void AuthService::recordFailedLogin(const std::string &ipAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto found = m_failedAttempts.find(ipAddress);
    if (found == m_failedAttempts.end()) {
        m_failedAttempts.insert(
                {ipAddress, {1, std::chrono::steady_clock::now()}});
        return;
    }
    found->second.m_failures++;
}

void AuthService::clearFailedLogins(const std::string &ipAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failedAttempts.erase(ipAddress);
}

bool AuthService::allowLoginAttempt(const std::string &ipAddress) {
    const auto now = std::chrono::steady_clock::now();
    auto &attempts = m_loginAttempts[ipAddress];
    while (!attempts.empty() &&
           (now - attempts.front()) >= m_rateLimitWindow->getValue()) {
        attempts.pop_front();
    }
    if (attempts.size() >= static_cast<std::size_t>(m_maxLoginAttempts->getValue())) {
        return false;
    }
    attempts.push_back(now);
    return true;
}

void AuthService::pruneLoginAttempts() {
    const auto now = std::chrono::steady_clock::now();
    for (auto iter = m_loginAttempts.begin(); iter != m_loginAttempts.end();) {
        while (!iter->second.empty() &&
               (now - iter->second.front()) >= m_rateLimitWindow->getValue()) {
            iter->second.pop_front();
        }
        if (iter->second.empty()) {
            iter = m_loginAttempts.erase(iter);
            continue;
        }
        ++iter;
    }
}

void AuthService::pruneFailedLogins() {
    const auto now = std::chrono::steady_clock::now();
    for (auto iter = m_failedAttempts.begin(); iter != m_failedAttempts.end();) {
        if ((now - iter->second.m_firstFailure) > m_loginLockout->getValue()) {
            iter = m_failedAttempts.erase(iter);
            continue;
        }
        ++iter;
    }
}
