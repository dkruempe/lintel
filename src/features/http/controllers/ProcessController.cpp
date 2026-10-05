#include "base_library/features/http/controllers/ProcessController.h"

#include "base_library/core/services/ProcessService.h"
#include "base_library/core/utils/RegexUtils.h"
#include "base_library/features/base/controller/ProcessGroupsDto.h"
#include "base_library/features/base/controller/ProcessInfosDto.h"
#include "base_library/features/base/models/Process.h"

ProcessController::ProcessController(const std::shared_ptr<IAuthService> &authService,
  std::shared_ptr<ProcessService> processService)
  : Controller(authService), m_processService(std::move(processService)), m_adminGroup("Admin-Process", {}, true),
    m_userGroup("User-Process", {}, true)
{
  add(m_adminGroup);
  add(m_userGroup);
}

void ProcessController::allProcessOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &userToken)
{
  // no user logged in => Unauthorized
  if (!userToken.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!userToken->m_user.has(m_userGroup) && !userToken->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    const std::string processName = request.matches[1];
    std::string errorMessage;
    if (!RegexUtils::validatePattern(processName, errorMessage)) {
      LOG_WARN("allProcessOfGet: {}", errorMessage);
      response.status = HttpStatusCodes::BadRequest;
      response.set_content("", contentType.getName());
      return;
    }
    const std::regex nameRegex(processName);
    std::vector<ProcessInfo> filtered;
    for (const auto &processInfo : m_processService->allActiveOf()) {
      const std::string path = processInfo.getProcess()->getPath().string();
      const std::string fileName = processInfo.getProcess()->getPath().filename().string();
      if (std::regex_match(path, nameRegex) || std::regex_match(fileName, nameRegex)) {
        filtered.push_back(processInfo);
      }
    }
    ProcessInfosDto processInfosDto(filtered);
    std::string content = processInfosDto.JsonSerializable::serialize();
    response.set_content(content, contentType.getName());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::allProcessGroupsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user logged in => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string groupName = request.matches[1];
  switch (contentType) {
  case ContentType::ApplicationJson: {
    auto temp = m_processService->allGroupsOf(groupName);
    ProcessGroupsDto dtos(temp);
    std::string content = dtos.JsonSerializable::serialize();
    response.set_content(content, contentType.getName());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::startProcessPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user logged in => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    ProcessInfoDto processInfoDto;
    processInfoDto.JsonSerializable::deserialize(request.body);
    Process process(processInfoDto.getPath(), processInfoDto.getArgs());
    if (processInfoDto.isAutoRestart()) {
      process.enableAutoStart(processInfoDto.getMaxAutoRestarts());
    } else {
      process.disableAutoStart();
    }
    m_processService->startOf(process);
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::stopProcessDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user logged in => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    std::string processId = request.matches[1];
    auto process = m_processService->of(processId);
    // unknown process id => the addressed resource does not exist
    if (!process.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      break;
    }
    m_processService->stopOf(*process.value());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::terminateProcessDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user logged in => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    std::string processId = request.matches[1];
    auto process = m_processService->of(processId);
    // unknown process id => the addressed resource does not exist
    if (!process.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      break;
    }
    LOG_INFO("terminate of {}", processId);
    m_processService->terminateOf(*process.value());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::restartProcessPut(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    std::string processId = request.matches[1];
    auto process = m_processService->of(processId);
    // unknown process id => the addressed resource does not exist
    if (!process.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      break;
    }
    LOG_INFO("restart of {}", processId);
    m_processService->restartOf(*process.value());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::resetProcessPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    std::string processId = request.matches[1];
    auto process = m_processService->of(processId);
    // unknown process id => the addressed resource does not exist
    if (!process.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      break;
    }
    LOG_INFO("reset of {}", processId);
    m_processService->resetOf(*process.value());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}

void ProcessController::healthProcessGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  switch (contentType) {
  case ContentType::ApplicationJson: {
    std::string processId = request.matches[1];
    auto process = m_processService->of(processId);
    // unknown process id => the addressed resource does not exist
    if (!process.has_value()) {
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
      break;
    }
    for (const auto &processInfo : m_processService->allActiveOf()) {
      if (processInfo.getProcess()->getId() != processId) { continue; }
      ProcessInfoDto processInfoDto(processInfo);
      response.set_content(processInfoDto.JsonSerializable::serialize(), contentType.getName());
      return;
    }
    // known, but no active process info left => the addressed resource does not exist (anymore)
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    break;
  }
  default: {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    break;
  }
  }
}
