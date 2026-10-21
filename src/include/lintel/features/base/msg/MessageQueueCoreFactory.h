#ifndef LINTEL_MESSAGEQUEUECOREFACTORY_H
#define LINTEL_MESSAGEQUEUECOREFACTORY_H

#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/msg/MessageQueueBackend.h"
#include "lintel/features/base/msg/MessageQueueCore.h"
#include "lintel/features/base/msg/MessageQueueShmCore.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace lintel {
/** Backend independent interface of a message queue core. */
class IMessageQueueCore
{
public:
  virtual ~IMessageQueueCore() = default;

  virtual void sendOf(const void *message, std::size_t size) = 0;
  virtual bool trySendOf(const void *message, std::size_t size) = 0;
  virtual bool sendOfWithTimeout(const void *message, std::size_t size, const std::chrono::milliseconds &timeout) = 0;
  virtual void receiveOf(void *message, std::size_t size) = 0;
  virtual bool tryReceiveOf(void *message, std::size_t size) = 0;
  virtual bool receiveOfWithTimeout(void *message, std::size_t size, const std::chrono::milliseconds &timeout) = 0;
  [[nodiscard]] virtual int32_t numberMessagesOf() const = 0;
  virtual void setOverflowPolicy(MessageQueueOverflowPolicy policy) = 0;
  virtual void setOverflowTimeout(const std::chrono::milliseconds &timeout) = 0;
  virtual void setDeadLetterQueueName(const std::string &name) = 0;
};

/** Adapter of the Boost (POSIX message queue) backend. */
class BoostMessageQueueCore final : public IMessageQueueCore
{
public:
  BoostMessageQueueCore(std::string name, int32_t msgCount, std::size_t msgSize)
    : m_core(std::move(name), msgCount, msgSize)
  {}

  void sendOf(const void *message, std::size_t size) override { m_core.sendOf(message, size); }

  bool trySendOf(const void *message, std::size_t size) override { return m_core.trySendOf(message, size); }

  bool sendOfWithTimeout(const void *message, std::size_t size, const std::chrono::milliseconds &timeout) override
  {
    return m_core.sendOfWithTimeout(message, size, timeout);
  }

  void receiveOf(void *message, std::size_t size) override { m_core.receiveOf(message, size); }

  bool tryReceiveOf(void *message, std::size_t size) override { return m_core.tryReceiveOf(message, size); }

  bool receiveOfWithTimeout(void *message, std::size_t size, const std::chrono::milliseconds &timeout) override
  {
    return m_core.receiveOfWithTimeout(message, size, timeout);
  }

  [[nodiscard]] int32_t numberMessagesOf() const override { return m_core.numberMessagesOf(); }

  void setOverflowPolicy(MessageQueueOverflowPolicy policy) override { m_core.setOverflowPolicy(policy); }

  void setOverflowTimeout(const std::chrono::milliseconds &timeout) override { m_core.setOverflowTimeout(timeout); }

  void setDeadLetterQueueName(const std::string &name) override { m_core.setDeadLetterQueueName(name); }

private:
  MessageQueueCore m_core;
};

/** Adapter of the shared memory (ring buffer) backend. */
class ShmMessageQueueCore final : public IMessageQueueCore
{
public:
  ShmMessageQueueCore(std::string name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
    : m_core(std::move(name), msgCount, msgSize, segment)
  {}

  void sendOf(const void *message, std::size_t size) override { m_core.sendOf(message, size); }

  bool trySendOf(const void *message, std::size_t size) override { return m_core.trySendOf(message, size); }

  bool sendOfWithTimeout(const void *message, std::size_t size, const std::chrono::milliseconds &timeout) override
  {
    return m_core.sendOfWithTimeout(message, size, timeout);
  }

  void receiveOf(void *message, std::size_t size) override { m_core.receiveOf(message, size); }

  bool tryReceiveOf(void *message, std::size_t size) override { return m_core.tryReceiveOf(message, size); }

  bool receiveOfWithTimeout(void *message, std::size_t size, const std::chrono::milliseconds &timeout) override
  {
    return m_core.receiveOfWithTimeout(message, size, timeout);
  }

  [[nodiscard]] int32_t numberMessagesOf() const override { return m_core.numberMessagesOf(); }

  void setOverflowPolicy(MessageQueueOverflowPolicy policy) override { m_core.setOverflowPolicy(policy); }

  void setOverflowTimeout(const std::chrono::milliseconds &timeout) override { m_core.setOverflowTimeout(timeout); }

  void setDeadLetterQueueName(const std::string &name) override { m_core.setDeadLetterQueueName(name); }

private:
  MessageQueueShmCore m_core;
};

/** Creates a message queue core for the selected backend. */
class MessageQueueCoreFactory
{
public:
  /** @param name queue name
   *  @param msgCount queue capacity in messages
   *  @param msgSize maximum message size in bytes
   *  @param segment shared memory segment (used by the Shm backend only)
   *  @param backend backend selection, defaults to the process wide config
   *  @return the created core */
  static std::unique_ptr<IMessageQueueCore> create(std::string name,
    int32_t msgCount,
    std::size_t msgSize,
    ShmSegmentAccessor segment,
    MessageQueueBackend backend = MessageQueueBackendConfig::backend())
  {
    if (backend == MessageQueueBackend::Shm) {
      return std::make_unique<ShmMessageQueueCore>(std::move(name), msgCount, msgSize, segment);
    }
    return std::make_unique<BoostMessageQueueCore>(std::move(name), msgCount, msgSize);
  }

  /** @param backend backend of the queue
   *  @param name queue name */
  static void removeOf(MessageQueueBackend backend, const std::string &name)
  {
    if (backend == MessageQueueBackend::Shm) {
      MessageQueueShmCore::removeOf(name);
    } else {
      MessageQueueCore::removeOf(name);
    }
  }

  /** @param backend backend of the queue
   *  @param name queue name
   *  @return number of queued messages, 0 if the queue does not exist */
  static int32_t numberMessagesOfIfExistsOf(MessageQueueBackend backend, const std::string &name)
  {
    if (backend == MessageQueueBackend::Shm) { return MessageQueueShmCore::numberMessagesOfIfExistsOf(name); }
    return MessageQueueCore::numberMessagesOfIfExistsOf(name);
  }
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUECOREFACTORY_H
