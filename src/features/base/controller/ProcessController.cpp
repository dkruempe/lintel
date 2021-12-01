#include "base_library/features/base/controller/ProcessController.h"

#include "base_library/core/services/ProcessService.h"
#include "base_library/features/base/controller/ProcessGroupsDto.h"
#include "base_library/features/base/controller/ProcessInfosDto.h"
#include "base_library/features/base/models/Process.h"

ProcessController::ProcessController(
    const std::shared_ptr<AuthService>& authService,
    std::shared_ptr<ProcessService> processService)
    : Controller(authService),
      m_processService(std::move(processService)),
      m_adminGroup("Admin-Process", {}, true),
      m_userGroup("User-Process", {}, true) {
  add(m_adminGroup);
  add(m_userGroup);
}

void ProcessController::allProcessOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
  // no user logged in => Unauthorized
  if (!userToken.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!userToken->m_user.has(m_userGroup) &&
      !userToken->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  const std::string processName = request.matches[1];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      ProcessInfosDto processInfosDto(m_processService->allActiveOf());
      std::string content = processInfosDto.JsonSerializable::serialize();
      LOG_TRACE("{}", content);
      response.set_content(content, contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void ProcessController::allProcessGroupsOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  // no user logged in => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  const std::string groupName = request.matches[1];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      auto temp = m_processService->allGroupsOf(groupName);
      ProcessGroupsDto dtos(temp);
      std::string content = dtos.JsonSerializable::serialize();
      LOG_TRACE("{}", content);
      response.set_content(content, contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
