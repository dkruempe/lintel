#ifndef CPP_BASE_LIBRARY_NOTIFICATION_H
#define CPP_BASE_LIBRARY_NOTIFICATION_H

#include "base_library/features/websocket/Message.h"

class Notification : public Message {
 public:
  std::string serialize() override;
  Notification() : Message() {}
  ~Notification() override = default;
};

#endif  // CPP_BASE_LIBRARY_NOTIFICATION_H
