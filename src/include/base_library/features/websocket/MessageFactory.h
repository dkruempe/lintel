#ifndef CPP_BASE_LIBRARY_MESSAGEFACTORY_H
#define CPP_BASE_LIBRARY_MESSAGEFACTORY_H

#include <memory>
#include <vector>

#include "base_library/features/websocket/Message.h"

class MessageFactory {
 public:
  static std::vector<std::shared_ptr<Message>> generate(
      const std::string& message);
};

#endif  // CPP_BASE_LIBRARY_MESSAGEFACTORY_H
