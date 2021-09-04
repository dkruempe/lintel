#include "base_library/features/base/services/AuthService.h"

#include <boost/asio.hpp>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/UUID.h"
#include "base_library/features/base/configuration/Cryption.h"

AuthService::AuthService(const std::shared_ptr<ProcessName>& processName,
                         std::shared_ptr<UserRepository> userRepository,
                         std::shared_ptr<SchedulerService> schedulerService)
    : AbstractService<AuthService>(processName->getProcessName()),
      m_userRepository(std::move(userRepository)),
      m_scheduler(std::move(schedulerService)) {}

std::optional<UserToken> AuthService::onLoginOf(const UserLogin& userLogin) {
  // I first easy check of ipaddress
  if (userLogin.m_ipAddress.empty()) {
    LOG_ERROR("ip address of {} empty", userLogin.m_userName);
    return std::nullopt;
  }
  // II check via boost ip address
  boost::system::error_code ec{};
  auto ip = boost::asio::ip::address::from_string(userLogin.m_ipAddress, ec);
  if (ip.to_string() != userLogin.m_ipAddress) {
    LOG_ERROR("{} invalid ip address of {}", userLogin.m_ipAddress,
              userLogin.m_userName);
    return std::nullopt;
  }
  // III check if user is logged in
  auto found = m_userTokens.find(userLogin.m_ipAddress);
  if (found != m_userTokens.end()) {
    LOG_INFO("{}: {} {} already logged in", userLogin.m_ipAddress,
             userLogin.m_userName, found->second.m_id);
    return std::make_optional(found->second);
  }
  auto userOpt = m_userRepository->of(userLogin.m_userName);
  if (!userOpt.has_value()) {
    LOG_INFO("{}: {} does not exists", userLogin.m_ipAddress,
             userLogin.m_userName);
    return std::nullopt;
  }
  const User& user = userOpt.value();
  if (user.getPassword() != Cryption::hashOf(userLogin.m_password)) {
    LOG_INFO("{}: {} password invalid", userLogin.m_ipAddress,
             userLogin.m_userName);
    return std::nullopt;
  }
  std::string id = UUID::generate();
  UserToken userToken{userLogin.m_ipAddress, id,
                      std::chrono::system_clock::now(), user};
  m_userTokens.insert({userToken.m_ipAddress, userToken});
  LOG_INFO("{}: {} {} success full login", userLogin.m_ipAddress,
           userLogin.m_userName, id);
  return userToken;
}
std::optional<UserToken> AuthService::onAccessOf(
    const UserTokenLogin& userTokenLogin) {
  auto found = m_userTokens.find(userTokenLogin.m_ipAddress);
  if (found == m_userTokens.end()) {
    LOG_ERROR("{}: {} user not logged in", userTokenLogin.m_ipAddress,
              userTokenLogin.m_id);
    return std::nullopt;
  }
  if (userTokenLogin.m_id != found->second.m_id) {
    m_userTokens.erase(found->first);
    LOG_ERROR("{}: {} user logged in but with different token != {}",
              userTokenLogin.m_ipAddress, userTokenLogin.m_id,
              found->second.m_id);
    return std::nullopt;
  }
  found->second.m_lastAccessTimestamps = std::chrono::system_clock::now();
  return std::make_optional(found->second);
}
void AuthService::onLogoutOf(const UserToken& userToken) {
  auto found = m_userTokens.find(userToken.m_ipAddress);
  if (found == m_userTokens.end()) {
    LOG_ERROR("{}: {} user not logged in", userToken.m_ipAddress,
              userToken.m_id);
    return;
  }
  m_userTokens.erase(userToken.m_ipAddress);
}
void AuthService::onCheck() {
  date::sys_time<std::chrono::microseconds> now =
      std::chrono::system_clock::now();
  for (auto& iter : m_userTokens) {
    if ((now - iter.second.m_lastAccessTimestamps) <
        m_timeoutLogin->getValue()) {
      continue;
    }
    m_userTokens.erase(iter.first);
  }
  m_scheduler->schedule_after(m_scheduleRate->getValue(), [&]() { onCheck(); });
}
void AuthService::onInitialize() {
  m_scheduler->schedule_after(m_scheduleRate->getValue(), [&]() { onCheck(); });
}