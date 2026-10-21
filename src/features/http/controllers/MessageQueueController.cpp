#include "lintel/features/http/controllers/MessageQueueController.h"

#include "lintel/features/base/controller/MessageQueueDtos.h"
#include "lintel/features/base/models/MessageQueueStats.h"

#include <exception>
#include <memory>

namespace lintel {
MessageQueueController::MessageQueueController(const std::shared_ptr<IAuthService> &authService,
  std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
  std::shared_ptr<MessageQueueService> messageQueueService)
  : Controller(authService), m_messageQueueRepository(std::move(messageQueueRepository)),
    m_messageQueueService(std::move(messageQueueService)), m_adminGroup("Admin-MessageQueue", {}, true),
    m_userGroup("User-MessageQueue", {}, true)
{
  add(m_adminGroup);
  add(m_userGroup);
}

bool MessageQueueController::canRead(const std::optional<UserToken> &user) const
{
  return user.has_value() && (user->m_user.has(m_userGroup) || user->m_user.has(m_adminGroup));
}

bool MessageQueueController::canAdminister(const std::optional<UserToken> &user) const
{
  return user.has_value() && user->m_user.has(m_adminGroup);
}

void MessageQueueController::writeMessageQueuesOf(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user,
  const std::string &processName,
  const std::string &messageQueueName)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!canRead(user)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  PagingParams paging = pagingParamsOf(request);
  if (paging.m_enabled) {
    Page<MessageQueueEntry> page = m_messageQueueRepository->pageOf(
      MessageQueueFilter{ processName, messageQueueName }, paging.m_after, paging.m_limit);
    MessageQueuesPageDto messageQueuesPageDto(page);
    response.set_content(messageQueuesPageDto.JsonSerializable::serialize(), contentType.getName());
    return;
  }
  std::vector<MessageQueueEntry> entries =
    m_messageQueueRepository->allOf(MessageQueueFilter{ processName, messageQueueName });
  std::vector<std::pair<MessageQueueEntry, int32_t>> messageQueueInformation;
  messageQueueInformation.reserve(entries.size());
  for (const auto &entry : entries) {
    messageQueueInformation.push_back(m_messageQueueService->numberMessagesOf(entry));
  }
  MessageQueueDtos messageQueueDtos(messageQueueInformation);
  response.set_content(messageQueueDtos.JsonSerializable::serialize(), contentType.getName());
}

void MessageQueueController::messageQueueOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  writeMessageQueuesOf(request, response, contentType, user, request.matches[1], request.matches[2]);
}

void MessageQueueController::statisticsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  writeMessageQueuesOf(request, response, contentType, user, request.matches[1], request.matches[2]);
}

void MessageQueueController::purgeOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  // purging is destructive, so only administrators may do it
  if (!canAdminister(user)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string requestedName = request.matches[1];
  std::unique_ptr<MessageQueueEntry> entry;
  try {
    entry = std::make_unique<MessageQueueEntry>(m_messageQueueRepository->allMessageQueueNameOf(requestedName));
  } catch (const std::exception &) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  // allMessageQueueNameOf() treats the name as a pattern: only an exact match
  // may be purged, so a wildcard can never wipe several queues at once.
  if (!entry || entry->get_message_queue_name() != requestedName) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  const int32_t purged = purgeMessageQueueOf(entry->get_message_queue_name(), entry->get_max_messages());
  MessageQueuePurgeDto result(entry->get_message_queue_name(), purged);
  response.set_content(result.JsonSerializable::serialize(), contentType.getName());
}
}// namespace lintel
