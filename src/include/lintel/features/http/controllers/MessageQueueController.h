#ifndef LINTEL_MESSAGEQUEUECONTROLLER_H
#define LINTEL_MESSAGEQUEUECONTROLLER_H
#include "lintel/features/base/repositories/IMessageQueueRepository.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/base/services/MessageQueueService.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/** HTTP controller that serves message queue information */
class MessageQueueController : public Controller
{
private:
  std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
  std::shared_ptr<MessageQueueService> m_messageQueueService;
  Group m_adminGroup;
  Group m_userGroup;

  /** Write the matching queues with their operational statistics as JSON.
   * @param request incoming request (for paging parameters)
   * @param response outgoing response
   * @param contentType negotiated content type
   * @param user authenticated user, if any
   * @param processName process name filter
   * @param messageQueueName queue name filter */
  void writeMessageQueuesOf(const httplib::Request &request,
    httplib::Response &response,
    const ContentType &contentType,
    const std::optional<UserToken> &user,
    const std::string &processName,
    const std::string &messageQueueName);

  /** @return true if the user may read queue information */
  bool canRead(const std::optional<UserToken> &user) const;

  /** @return true if the user may mutate queue state */
  bool canAdminister(const std::optional<UserToken> &user) const;

  ADD_HANDLER_METHOD(R"(/messageQueue/([^\/]+)/([^\/]+))", Get, messageQueueOf);
  ADD_HANDLER_METHOD(R"(/messageQueue/stats/([^\/]+)/([^\/]+))", Get, statisticsOf);
  ADD_HANDLER_METHOD(R"(/messageQueue/purge/([^\/]+))", Post, purgeOf);

public:
  explicit MessageQueueController(const std::shared_ptr<IAuthService> &authService,
    std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
    std::shared_ptr<MessageQueueService> messageQueueService);
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUECONTROLLER_H
