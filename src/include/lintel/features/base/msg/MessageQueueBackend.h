#ifndef LINTEL_MESSAGEQUEUEBACKEND_H
#define LINTEL_MESSAGEQUEUEBACKEND_H

#include <atomic>
#include <cstdlib>
#include <string_view>

namespace lintel {
/** Selectable transport backend of a message queue. */
enum class MessageQueueBackend { Boost = 0, Shm };

/**
 * Process wide default backend selection for message queues.
 *
 * This is deliberately a small feature flag: existing MessageQueue<T> users
 * keep the Boost backend unless they explicitly opt into the shared memory
 * queue (or use MessageQueueShmCore directly). The value can be set from code
 * or from the LINTEL_MESSAGE_QUEUE_BACKEND environment variable
 * ("boost" or "shm").
 */
class MessageQueueBackendConfig
{
public:
  /** @param backend the backend used for newly created queues */
  static void setBackend(MessageQueueBackend backend) noexcept
  {
    backendStorage().store(backend, std::memory_order_relaxed);
  }

  /** @return the configured backend */
  [[nodiscard]] static MessageQueueBackend backend() noexcept
  {
    return backendStorage().load(std::memory_order_relaxed);
  }

  /** @return true when the shared memory backend is selected */
  [[nodiscard]] static bool useSharedMemory() noexcept { return backend() == MessageQueueBackend::Shm; }

  /** @param name "boost" or "shm" (case insensitive)
   *  @return the parsed backend, Boost for an unknown name */
  [[nodiscard]] static MessageQueueBackend fromString(std::string_view name) noexcept
  {
    if (name == "shm" || name == "SHM" || name == "Shm") { return MessageQueueBackend::Shm; }
    return MessageQueueBackend::Boost;
  }

  /** Apply the LINTEL_MESSAGE_QUEUE_BACKEND environment variable when set.
   *  @return the resulting backend */
  static MessageQueueBackend configureFromEnvironment() noexcept
  {
    if (const char *value = std::getenv("LINTEL_MESSAGE_QUEUE_BACKEND"); value != nullptr) {
      setBackend(fromString(value));
    }
    return backend();
  }

private:
  static std::atomic<MessageQueueBackend> &backendStorage() noexcept
  {
    static std::atomic<MessageQueueBackend> backend{ MessageQueueBackend::Boost };
    return backend;
  }
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEBACKEND_H
