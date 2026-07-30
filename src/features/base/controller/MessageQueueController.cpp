#include "base_library/features/base/controller/MessageQueueController.h"

#include "base_library/features/base/controller/MessageQueueDtos.h"

MessageQueueController::MessageQueueController(const std::shared_ptr<IAuthService> &authService,
  std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
  std::shared_ptr<MessageQueueService> messageQueueService)
  : Controller(authService),
    m_messageQueueRepository(std::move(messageQueueRepository)),
    m_messageQueueService(std::move(messageQueueService)),
    m_adminGroup("Admin-MessageQueue", {}, true),
    m_userGroup("User-MessageQueue", {}, true) {
  add(m_adminGroup);
  add(m_userGroup);
}

void MessageQueueController::messageQueueOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user) {
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
  const std::string processName = request.matches[1];
  const std::string messageQueueName = request.matches[2];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      std::vector<MessageQueueEntry> entries = m_messageQueueRepository->allOf(processName, messageQueueName);
      std::vector<std::pair<MessageQueueEntry, int32_t> > messageQueueInformation;
      for (const auto &entry : entries) { messageQueueInformation.push_back(m_messageQueueService->numberMessagesOf(entry)); }
      MessageQueueDtos messageQueueDtos(messageQueueInformation);
      response.set_content(messageQueueDtos.JsonSerializable::serialize(), contentType.getName());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      break;
    }
  }
}