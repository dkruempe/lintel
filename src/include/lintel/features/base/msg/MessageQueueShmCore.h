#ifndef LINTEL_MESSAGEQUEUESHMCORE_H
#define LINTEL_MESSAGEQUEUESHMCORE_H

#include <chrono>
#include <cstddef>
#include <string>

namespace lintel {

class MessageQueueShmCore
{
public:
  MessageQueueShmCore() = default;
  ~MessageQueueShmCore() = default;
  static void removeOf(const std::string &) {}
};

}// namespace lintel
#endif
