#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUECONTROLLER_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUECONTROLLER_H
#include "base_library/features/base/repositories/MessageQueueRepository.h"
#include "base_library/features/base/services/MessageQueueService.h"
#include "base_library/features/http/service/Controller.h"

class MessageQueueController : public Controller
{
private:
  std::shared_ptr<MessageQueueRepository> m_messageQueueRepository;
  std::shared_ptr<MessageQueueService> m_messageQueueService;
  Group m_adminGroup;
  Group m_userGroup;

  ADD_HANDLER_METHOD(R"(/messageQueue/([^\/]+)/([^\/]+))", Get, messageQueueOf);

public:
  explicit MessageQueueController(const std::shared_ptr<AuthService> &authService,
    std::shared_ptr<MessageQueueRepository> messageQueueRepository,
    std::shared_ptr<MessageQueueService> messageQueueService);
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUECONTROLLER_H