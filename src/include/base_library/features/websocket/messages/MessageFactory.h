#ifndef CPP_BASE_LIBRARY_MESSAGEFACTORY_H
#define CPP_BASE_LIBRARY_MESSAGEFACTORY_H

#include <rapidjson/document.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "base_library/features/websocket/messages/ErrorMessage.h"
#include "base_library/features/websocket/messages/Message.h"
#include "base_library/features/websocket/models/ErrorCode.h"
#include "base_library/features/websocket/models/MessageContainer.h"
#include "base_library/features/websocket/models/ProcessingRequests.h"

class MessageFactory {
 private:
  typedef std::function<std::shared_ptr<Message>(
      rapidjson::GenericValue<rapidjson::UTF8<>,
                              rapidjson::MemoryPoolAllocator<>> *)>
      create_t;
  std::map<std::string, create_t> m_requests;   // requests and notification
  std::map<std::string, create_t> m_responses;  // responses of requests

  static std::shared_ptr<ErrorMessage> createErrorMessage(ErrorCode errorCode,
                                                          std::string message,
                                                          std::string id = "");

 public:
  void registerRequest(const std::string &method,
                       const create_t &createFunction);
  void registerResponse(const std::string &method,
                        const create_t &createFunction);
  MessageContainer generate(const std::string &message,
                            ProcessingRequests &processingRequests);
};

#endif  // CPP_BASE_LIBRARY_MESSAGEFACTORY_H
