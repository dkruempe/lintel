#ifndef CPP_BASE_LIBRARY_MESSAGEFACTORY_H
#define CPP_BASE_LIBRARY_MESSAGEFACTORY_H

#include <memory>
#include <vector>

#include "Message.h"
#include "base_library/features/websocket/models/MessageContainer.h"

class MessageFactory {
 public:
  static MessageContainer generate(
      const std::string& message);
};

#endif  // CPP_BASE_LIBRARY_MESSAGEFACTORY_H
