#ifndef CPP_BASE_LIBRARY_RESPONSE_H
#define CPP_BASE_LIBRARY_RESPONSE_H

#include "base_library/features/websocket/Message.h"

class Response : public Message {
 public:
  std::string serialize() override;
  Response() : Message() {}
  ~Response() = default;
};

#endif  // CPP_BASE_LIBRARY_RESPONSE_H
