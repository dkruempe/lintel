#ifndef LINTEL_MESSAGEQUEUECONTROLLER_H
#define LINTEL_MESSAGEQUEUECONTROLLER_H
#include "lintel/features/base/repositories/IMessageQueueRepository.h"
#include "lintel/features/base/services/MessageQueueService.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/Controller.h"

/** HTTP controller that serves message queue information */
class MessageQueueController : public Controller
{
private:
  std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
  std::shared_ptr<MessageQueueService> m_messageQueueService;
  Group m_adminGroup;
  Group m_userGroup;

  ADD_HANDLER_METHOD(R"(/messageQueue/([^\/]+)/([^\/]+))", Get, messageQueueOf);

public:
  explicit MessageQueueController(const std::shared_ptr<IAuthService> &authService,
    std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
    std::shared_ptr<MessageQueueService> messageQueueService);
};

#endif //LINTEL_MESSAGEQUEUECONTROLLER_H
