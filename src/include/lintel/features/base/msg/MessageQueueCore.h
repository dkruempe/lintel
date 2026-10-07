#ifndef LINTEL_MESSAGEQUEUECORE_H
#define LINTEL_MESSAGEQUEUECORE_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/**
 * Non-template core of MessageQueue. Owns the underlying POSIX message queue,
 * so translation units that use MessageQueue<T> do not have to parse the
 * boost::interprocess headers. Messages are passed as byte buffers whose size
 * the caller (MessageQueue<T>) derives from its own type.
 */
class MessageQueueCore
{
private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  /**
   * open or create the message queue
   *
   * @param name of the message queue. No filepath just name.
   * @param msgCount limit of queue itself
   * @param msgSize maximum size of a single message in bytes
   */
  MessageQueueCore(std::string name, int32_t msgCount, std::size_t msgSize);

  ~MessageQueueCore();

  MessageQueueCore(const MessageQueueCore &) = delete;

  MessageQueueCore &operator=(const MessageQueueCore &) = delete;

  /**
   * remove of message queue
   *
   * @param messageQueueName name of the queue to remove
   */
  static void removeOf(const std::string &messageQueueName);

  /**
   * Open an existing queue without creating it and read its message count.
   *
   * @param messageQueueName name of the queue to inspect
   * @return number of messages, 0 if the queue does not exist
   */
  static int32_t numberMessagesOfIfExistsOf(const std::string &messageQueueName);

  /** @return current number of messages in the queue */
  [[nodiscard]] int32_t numberMessagesOf() const;

  /**
   * send of message. Message Queue full => blocks until message queue is free
   *
   * @param message buffer holding the message
   * @param size of the message in bytes
   */
  void sendOf(const void *message, std::size_t size);

  /**
   * send non blocking. Message Queue full => return false and doesn't send
   * message.
   *
   * @param message buffer holding the message
   * @param size of the message in bytes
   * @return true if the message was queued
   */
  bool trySendOf(const void *message, std::size_t size);

  /**
   * send with a bounded wait. If the message queue stays full for the whole
   * timeout the message is dropped.
   *
   * @param message buffer holding the message
   * @param size of the message in bytes
   * @param timeout maximum time to wait for queue space
   * @return true if the message was queued, false if it was dropped
   */
  bool sendOfWithTimeout(const void *message, std::size_t size, const std::chrono::milliseconds &timeout);

  /**
   * receives message. No message => waits until message is available
   *
   * @param message buffer receiving the message
   * @param size maximum size of the received message in bytes
   */
  void receiveOf(void *message, std::size_t size);

  /**
   * try to receive message. No message => non blocking and return false
   *
   * @param message buffer receiving the message
   * @param size maximum size of the received message in bytes
   * @return true if a message was received
   */
  bool tryReceiveOf(void *message, std::size_t size);
};

#endif// LINTEL_MESSAGEQUEUECORE_H
