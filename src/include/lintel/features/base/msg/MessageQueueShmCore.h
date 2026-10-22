#ifndef LINTEL_MESSAGEQUEUESHMCORE_H
#define LINTEL_MESSAGEQUEUESHMCORE_H

#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/msg/MessageQueueCore.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace lintel {
/**
 * Shared memory backed message queue with the same API as MessageQueueCore.
 *
 * The queue is a bounded multi-producer / multi-consumer ring of fixed size
 * byte slots. The ring state (head, tail, slot sequences, slot size and
 * capacity) and the slot array live inside the shared memory segment the
 * accessor points to. The slots are addressed through a byte offset relative
 * to the ring, so the layout is identical in every mapping of the segment and
 * no process local pointer leaks into shared memory.
 *
 * Messages are copied into a slot of exactly `msgSize` bytes. No heap
 * allocation is involved per send/receive; the bytes stay in the shared
 * segment (hence "zero-copy" in the sense of the SHM storage model).
 *
 * The queue is shared across processes: a header that describes the ring
 * layout is stored in the segment under a name derived from the queue name, so
 * the second process finds the ring the first one created instead of building
 * one of its own. The segment accessor therefore has to be a *named* one (see
 * SharedMemoryService::getSegmentAccessor) - an unnamed accessor is bound to a
 * single process and is rejected.
 *
 * Instances of one process that use the same queue name share the same state:
 * a queue created by one core is found by every later core until the last one
 * is destroyed (or removeOf() is called). Queue names are process wide unique,
 * matching the naming of the Boost backend.
 *
 * Waiters poll in slices of about a millisecond: the condition variable only
 * reaches the threads of one process, and a producer in another process cannot
 * signal it (see QueueState::waitForProgress).
 */
class MessageQueueShmCore
{
private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  /**
   * open or create the shared memory message queue
   *
   * The queue is looked up in the segment by name: a core created by another
   * process is found and adopted, including its layout. The ring is destroyed
   * by the process that created it.
   *
   * @param name of the message queue. No filepath just name; at most 21
   *        characters, because the segment object name adds the "lintel_mq_"
   *        prefix and is limited to 32 characters including its NUL.
   * @param msgCount capacity of the queue in messages (must be positive)
   * @param msgSize maximum size of a single message in bytes (must be positive)
   * @param segment named accessor of the shared memory segment holding the queue
   * @throws std::invalid_argument on invalid size/capacity, on an unnamed
   *         segment accessor, on a queue name that is too long, or when an
   *         existing queue with the same name uses a different layout
   */
  MessageQueueShmCore(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment);

  ~MessageQueueShmCore();

  MessageQueueShmCore(const MessageQueueShmCore &) = delete;

  MessageQueueShmCore &operator=(const MessageQueueShmCore &) = delete;

  /**
   * remove of message queue (drops the process local association; the segment
   * itself owns the memory)
   *
   * The ring stays in the segment as long as a core of the creating process is
   * alive; that core marks the header uninitialised when it goes away, so a
   * later open re-creates the ring instead of adopting a destroyed one.
   *
   * @param messageQueueName name of the queue to remove
   */
  static void removeOf(const std::string &messageQueueName);

  /**
   * Look up an existing queue without creating it and read its message count.
   *
   * The lookup is process local: it answers for the cores this process has
   * open. A queue that another process created and this one never opened is
   * reported as 0, because the count lives behind an accessor this call does
   * not have.
   *
   * @param messageQueueName name of the queue to inspect
   * @return number of messages, 0 if the queue does not exist
   */
  static int32_t numberMessagesOfIfExistsOf(const std::string &messageQueueName);

  /** @return current number of messages in the queue */
  [[nodiscard]] int32_t numberMessagesOf() const;

  /** @return name of the queue */
  [[nodiscard]] const std::string &getName() const;

  /** @return maximum number of messages the queue can hold */
  [[nodiscard]] int32_t getMsgCount() const;

  /** @return maximum size of one message in bytes */
  [[nodiscard]] std::size_t getMsgSize() const;

  /** @return true if the ring operations are lock-free */
  [[nodiscard]] static bool isLockFree();

  /**
   * send of message. Message Queue full => blocks until message queue is free
   *
   * @param message buffer holding the message
   * @param size of the message in bytes
   */
  void sendOf(const void *message, std::size_t size);

  /**
   * send non blocking. Message Queue full => return false and doesn't send
   * message. The configured overflow policy is applied (DropOldest replaces
   * the oldest message, DeadLetter forwards the message to the DLQ).
   *
   * @param message buffer holding the message
   * @param size of the message in bytes
   * @return true if the message was queued (or forwarded to the DLQ)
   */
  bool trySendOf(const void *message, std::size_t size);

  /**
   * send with a bounded wait. If the message queue stays full for the whole
   * timeout the overflow policy is applied (DropOldest/DeadLetter) or the
   * message is dropped.
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

  /**
   * receives message with a bounded wait. No message within the timeout =>
   * return false.
   *
   * @param message buffer receiving the message
   * @param size maximum size of the received message in bytes
   * @param timeout maximum time to wait for a message
   * @return true if a message was received
   */
  bool receiveOfWithTimeout(void *message, std::size_t size, const std::chrono::milliseconds &timeout);

  /** Configure overflow policy. */
  void setOverflowPolicy(MessageQueueOverflowPolicy policy);
  /** Configure timeout for BlockWithTimeout policy. */
  void setOverflowTimeout(const std::chrono::milliseconds &timeout);
  /** Configure dead letter queue name. */
  void setDeadLetterQueueName(const std::string &name);
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUESHMCORE_H
