#ifndef LINTEL_MESSAGEQUEUEOWNER_H
#define LINTEL_MESSAGEQUEUEOWNER_H

#include <chrono>
#include <cstdint>
#include <string>

namespace lintel {

/** Owner/liveness information for a message queue. */
struct MessageQueueOwner
{
  std::string queueName;
  std::string processName;
  std::string hostName;
  uint64_t pid = 0;
  std::chrono::system_clock::time_point startedAt;
  std::chrono::system_clock::time_point lastHeartbeatAt;
  bool alive = true;
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEOWNER_H
